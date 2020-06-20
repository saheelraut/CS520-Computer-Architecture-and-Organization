/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Saheel Raut (sraut1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/* Set this flag to 1 to enable debug messages */
int ENABLE_DEBUG_MESSAGES;
#define DISPLAY 1
int HALT_FLAG = 0;
/*
 * This function creates and initializes APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
APEX_CPU*
APEX_cpu_init(const char *filename, const char *simulate, const int clockcycles)
{
  if (!filename)
  {
    return NULL;
  }

  APEX_CPU* cpu = malloc(sizeof(*cpu));
  if (!cpu)
  {
    return NULL;
  }

  if (strcmp(simulate, "display") == 0)
  {
    ENABLE_DEBUG_MESSAGES = 0;
  }
  else
  {
    ENABLE_DEBUG_MESSAGES = 1;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;
  memset(cpu->regs, 0, sizeof(int) * 16);
  memset(cpu->regs_valid, 1, sizeof(int) * 16);
  memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);

  for (int i = 0; i < 16; i++)
  {
    cpu->regs_valid[i] = 1;
  }

  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  /*Initializing clock cycle to 1*/
  cpu->clock = 0;

   /* Setting the z flag to 0 */
  cpu->z_flag_set = 1;
  cpu->clockcycles = clockcycles;
  if (!cpu->code_memory)
  {
    free(cpu);
    return NULL;
  }

  if (ENABLE_DEBUG_MESSAGES)
  {
    fprintf(stderr,
            "APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",
            cpu->code_memory_size);
    fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
    printf("%-9s %-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2","rs3","imm");

    for (int i = 0; i < cpu->code_memory_size; ++i)
    {
      printf("%-9s %-9d %-9d %-9d %-9d %-9d \n",
             cpu->code_memory[i].opcode,
             cpu->code_memory[i].rd,
             cpu->code_memory[i].rs1,
             cpu->code_memory[i].rs2,
             cpu->code_memory[i].rs3,
             cpu->code_memory[i].imm);
    }
  }

  /* Make all stages busy except Fetch stage, initally to start the pipeline */
  for (int i = 1; i < NUM_STAGES; ++i)
  {
    cpu->stage[i].busy = 1;
  }

  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
void APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 *
 * Note : You are not supposed to edit this function
 *
 */
int get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

static void
print_instruction(CPU_Stage* stage)
{
  if (strcmp(stage->opcode, "STORE") == 0)
  {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }
  if (strcmp(stage->opcode, "LOAD") == 0)
  {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "STR") == 0)
  {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->rs3);
  }

  if (strcmp(stage->opcode, "ADDL") == 0 ||
      strcmp(stage->opcode, "SUBL") == 0)
  {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "ADD") == 0 ||
      strcmp(stage->opcode, "SUB") == 0 ||
      strcmp(stage->opcode, "AND") == 0 ||
      strcmp(stage->opcode, "OR") == 0 ||
      strcmp(stage->opcode, "EX-OR") == 0 ||
      strcmp(stage->opcode, "MUL") == 0||
      strcmp(stage->opcode, "LDR") == 0)
  {

    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }

  if (strcmp(stage->opcode, "MOVC") == 0)
  {
    printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
  }
  if (strcmp(stage->opcode, "BZ") == 0 ||
      strcmp(stage->opcode, "BNZ") == 0)
  {
    printf("%s,#%d ", stage->opcode, stage->imm);
  }
  if (strcmp(stage->opcode, "JUMP") == 0)
  {
    printf("%s,R%d,#%d ", stage->opcode, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "HALT") == 0)
  {
    printf("%s", stage->opcode);
  }

  if (strcmp(stage->opcode, "EMPTY") == 0)
  {
    printf("%s", stage->opcode);
  }
}

/* Debug function which dumps the cpu stage
 * content
 *
 * Note : You are not supposed to edit this function
 *
 */
static void
print_stage_content(char* name, CPU_Stage* stage)
{
  printf("%-15s: pc(%d) ", name, stage->pc);
  print_instruction(stage);
  printf("\n");
}
static void
display(APEX_CPU* cpu)
{
  printf("\n");
printf("==================REGISTER VALUE==============");
  for(int i=0;i<16;i++)
  {
    printf("\n");
    printf(" | Register[%d] | Value=%d | status=%s |",i,cpu->regs[i],(cpu->regs_valid[i])?"Valid" : "Invalid");
  }

printf("\n");
printf("==================DATA MEMORY ==============");
printf("\n");
  for(int i=0;i<99;i++)
  {
    printf(" | MEM[%d] | Value=%d | \n",i,cpu->data_memory[i]);
  }
}

/*
 *  Fetch Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  if (!stage->busy && !stage->stalled)
  {
    /* Store current PC in fetch latch */
    stage->pc = cpu->pc;

    /* Index into code memory using this pc and copy all instruction fields into
     * fetch latch
     */
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
    strcpy(stage->opcode, current_ins->opcode);
    stage->rd = current_ins->rd;
    stage->rs1 = current_ins->rs1;
    stage->rs2 = current_ins->rs2;
    stage->rs3 = current_ins->rs3;
    stage->imm = current_ins->imm;
    stage->rd = current_ins->rd;

    /* Update PC for next instruction */
    cpu->pc += 4;

    /* Copy data from fetch latch to decode latch*/
    if (!cpu->stage[DRF].stalled)
    {
      cpu->stage[DRF] = cpu->stage[F];
      cpu->stage[F].stalled=0;
    }
    else
    {
      stage->stalled = 1;
    }
    if (ENABLE_DEBUG_MESSAGES) 
    {
      print_stage_content("Fetch Stage", stage);
    }
  }
    else
    {

      if (strcmp(stage->opcode,"EMPTY") == 0)
      {
        stage->stalled = 0;
        if (ENABLE_DEBUG_MESSAGES)
         {
          print_stage_content("Fetch Stage", stage);
         }
      }


      if (!cpu->stage[DRF].stalled && strcmp(cpu->stage[DRF].opcode, "EMPTY") != 0) 
      {
        stage->stalled = 0;
        cpu->stage[DRF] = cpu->stage[F];
        if (ENABLE_DEBUG_MESSAGES)
        {
          print_stage_content("Fetch", stage);
        }
    }

    /* Show if next stage is not HALT */
    if (cpu->stage[DRF].stalled && strcmp(cpu->stage[DRF].opcode, "HALT") != 0) {
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Fetch", stage);
      }
    }
      
    }
    
return 0;
}

/*
 *  Decode Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int decode(APEX_CPU* cpu)
{
   CPU_Stage* stage = &cpu->stage[DRF];
  if (!stage->busy && !stage->stalled)
    {

    /* Read data from register file for store */
    if (strcmp(stage->opcode, "STORE") == 0)
    {
      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2])
      {
        stage->stalled = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
      }
      else
      {
        stage->stalled = 1;
      }
    }

    /* STR */
    if (strcmp(stage->opcode, "STR") == 0)
    {
      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2] && cpu->regs_valid[stage->rs3])
      {
        stage->stalled = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
        stage->rs3_value = cpu->regs[stage->rs3];
      }
      else
      {
        stage->stalled = 1;
      }
    }

    /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0)
    {
      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rd])
      {
        stage->stalled = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        cpu->regs_valid[stage->rd] = 0;   //making register invalid
      }
      else
      {
        stage->stalled = 1;
      }
    }

    /* LDR */
    if (strcmp(stage->opcode, "LDR") == 0)
    {
      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2] && cpu->regs_valid[stage->rd])
      {
        stage->stalled = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs1_value = cpu->regs[stage->rs2];
        cpu->regs_valid[stage->rd] = 0;
      }
      else
      {
        stage->stalled = 1;
      }
    }

    /* No Register file read needed for MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0)
    {
      cpu->regs_valid[stage->rd] = 0;
    }

    /* ADD, SUB , MUL*/
    if (strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "MUL") == 0 ||
        strcmp(stage->opcode, "ADDL") == 0 ||
        strcmp(stage->opcode, "SUBL") == 0 )
    {
      cpu->z_flag_set = 0;
      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2] && cpu->regs_valid[stage->rd])
      {
        stage->stalled = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
        cpu->regs_valid[stage->rd] = 0;
        cpu->z_flag_set = 0;
      }
      else
      {
        stage->stalled = 1;
      }
    }

    /* AND,OR,EX-OR */
    if (strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0)
    {
      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2] && cpu->regs_valid[stage->rd])
      {
        stage->stalled = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
        cpu->regs_valid[stage->rd] = 0;
      }
      else
      {
        stage->stalled = 1;
      }
    }

    /* JUMP */
    if (strcmp(stage->opcode, "JUMP") == 0)
    {
      if (cpu->regs_valid[stage->rs1]) 
      {
        stage->stalled = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
      }
      else 
      {
        stage->stalled = 1;
      }
    }

    /* HALT */
    if (strcmp(stage->opcode, "HALT") == 0)
    {
      cpu->stage[F].stalled = 1;
      cpu->stage[F].pc = 0;
      cpu->stage[F].busy = 1;
    }

    /* BZ and BNZ */
    if (strcmp(stage->opcode, "BZ") == 0 ||
     strcmp(stage->opcode, "BNZ") == 0)
    {
      if (!cpu->z_flag_set)
      {
        stage->stalled = 1;
      }
    }
    if (ENABLE_DEBUG_MESSAGES) 
    {
      print_stage_content("Decode/RF Stage", stage);
    }

    /* Copy data from decode latch to Execute 1 latch*/
    
    if (!stage->stalled) {
      cpu->stage[EX1] = cpu->stage[DRF];
    }
    else
    {
      strcpy(cpu->stage[EX1].opcode, "EMPTY");
      cpu->stage[EX1].pc = 0;
    }
  }
  else
  {
    if (stage->stalled && strcmp(stage->opcode, "HALT") != 0 && !cpu->stage[EX1].stalled)
    {
      if (strcmp(stage->opcode, "STORE") == 0 ||
          strcmp(stage->opcode, "STR") == 0)
      {
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
        stage->stalled = 0;
        cpu->stage[EX1] = cpu->stage[DRF];
      }
      if (strcmp(stage->opcode, "ADD") == 0 ||
          strcmp(stage->opcode, "ADDL") == 0 ||
          strcmp(stage->opcode, "SUB") == 0 ||
          strcmp(stage->opcode, "SUBL") == 0 ||
          strcmp(stage->opcode, "OR") == 0 ||
          strcmp(stage->opcode, "EX-OR") == 0 ||
          strcmp(stage->opcode, "MUL") == 0 ||
          strcmp(stage->opcode, "AND") == 0)
      {
        cpu->regs_valid[stage->rd] = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
        stage->stalled = 0;
        cpu->stage[EX1] = cpu->stage[DRF];
      }

      if (strcmp(stage->opcode, "LOAD") == 0)
      {
        if (cpu->regs_valid[stage->rs1])
        {
          cpu->regs_valid[stage->rd] = 0;
          stage->rs1_value = cpu->regs[stage->rs1];
          stage->stalled = 0;
          cpu->stage[EX1] = cpu->stage[DRF];
        }
      }

      if (strcmp(stage->opcode, "JUMP") == 0)
      {
        if (cpu->regs_valid[stage->rs1])
        {
          stage->rs1_value = cpu->regs[stage->rs1];
          stage->stalled = 0;
          cpu->stage[EX1] = cpu->stage[DRF];
        }
      }
      if (strcmp(stage->opcode, "BZ") == 0 ||
          strcmp(stage->opcode, "BNZ") == 0)
      {
          stage->stalled = 0;
          cpu->stage[EX1] = cpu->stage[DRF];
      }

      if (ENABLE_DEBUG_MESSAGES)
      {
        print_stage_content("Decode/RF", stage);
      }

      if (cpu->stage[EX1].stalled && strcmp(cpu->stage[EX1].opcode, "HALT") != 0)
      {
        if (ENABLE_DEBUG_MESSAGES)
        {
          print_stage_content("Decode/RF", stage);
        }
      }

      //TODO HALTING
    }

    return 0;
  }
}

/*
 *  Execute 1 Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int execute1(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX1];
  if (!stage->busy && !stage->stalled)
  {

    /* Store */
    if (strcmp(stage->opcode, "STORE") == 0)
    {
      stage->mem_address = stage->rs2_value + stage->imm;
    }

    /* STR */
    if (strcmp(stage->opcode, "STR") == 0)
    {
      stage->mem_address = stage->rs2_value + stage->rs3_value;
    }

    /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0)
    {
      stage->mem_address = stage->rs1_value + stage->imm;
    }

    /* LDR */
    if (strcmp(stage->opcode, "LDR") == 0)
    {
      stage->mem_address = stage->rs1_value + stage->rs2_value;
    }

    /* MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0)
    {
      stage->buffer = stage->imm + 0;
    }

    /* ADD */
    if (strcmp(stage->opcode, "ADD") == 0)
    {
      stage->buffer = stage->rs1_value + stage->rs2_value;

      if(stage->buffer == 0)
       {
         cpu->z_flag_set = 1;
       }
       else
       {
         cpu->z_flag_set = 0;
       }
    }

    /* SUB */
    if (strcmp(stage->opcode, "SUB") == 0)
    {
      stage->buffer = stage->rs1_value - stage->rs2_value;
      if(stage->buffer == 0)
       {
         cpu->z_flag_set = 1;
       }
       else
       {
         cpu->z_flag_set = 0;
       }
    }

    /* AND */
    if (strcmp(stage->opcode, "AND") == 0)
    {
      stage->buffer = stage->rs1_value & stage->rs2_value;
    }

    /* OR */
    if (strcmp(stage->opcode, "OR") == 0)
    {
      stage->buffer = stage->rs1_value | stage->rs2_value;
    }

    /* EX-OR */
    if (strcmp(stage->opcode, "EX-OR") == 0)
    {
      stage->buffer = stage->rs1_value ^ stage->rs2_value;
    }

    /* MUL */
    if (strcmp(stage->opcode, "MUL") == 0)
    {
      stage->buffer = stage->rs1_value * stage->rs2_value;

      if(stage->buffer == 0)
       {
         cpu->z_flag_set = 1;
       }
       else
       {
         cpu->z_flag_set = 0;
       }
    }

    /* JUMP */
    if (strcmp(stage->opcode, "JUMP") == 0)
    {
      stage->buffer = stage->rs1_value + stage->imm;
    }

    /* HALT */
    if (strcmp(stage->opcode, "HALT") == 0)
    {
      //stage->stalled = 1;
      cpu->stage[DRF].stalled = 1;
      cpu->stage[DRF].pc = 0;
      cpu->stage[F].busy = 1;
    }

    /* BZ */
    if (strcmp(stage->opcode, "BZ") == 0)
    {
      if (cpu->z_flag_set == 1) 
      {
        stage->buffer = stage->pc + stage->imm;
        cpu->z_flag_set = 0;
      }

    }

    /* BNZ */
    if (strcmp(stage->opcode, "BNZ") == 0)
    {
      if (cpu->z_flag_set == 1) 
      {
        stage->buffer = stage->pc + stage->imm;
      }

    }

    /* Copy data from Execute 1 latch to Execute 2 latch*/
    if (!stage->stalled)
    {
      cpu->stage[EX2] = cpu->stage[EX1];
    }
    else
    {
      cpu->stage[DRF].stalled = 1;
      strcpy(cpu->stage[EX2].opcode, "EMPTY");
      cpu->stage[EX2].pc = 0;
    }
    
    if (ENABLE_DEBUG_MESSAGES)
    {
      print_stage_content("Execute 1 Stage", stage);
    }
  }
  return 0;
}

/*
 *  Execute 2 Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int execute2(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX2];
  if (!stage->busy && !stage->stalled)
  {

    /* Copy data from Execute 2 latch to Memory 1 latch*/
    if (!stage->stalled)
    {
      cpu->stage[MEM1] = cpu->stage[EX2];
    }
    else
    {
      cpu->stage[EX1].stalled = 1;
      strcpy(cpu->stage[MEM1].opcode, "EMPTY");
      cpu->stage[MEM1].pc = 0;
    }
    

    if (strcmp(stage->opcode, "HALT") == 0) {
      //stage->stalled = 1;
      cpu->stage[EX1].stalled = 1;
      cpu->stage[EX1].pc = 0;
      cpu->stage[F].busy = 1;
    }

    if (strcmp(stage->opcode, "MOVC") == 0 ||
        strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0 ||
        strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "ADDL") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "SUBL") == 0 ||
        strcmp(stage->opcode, "MUL") == 0 )
    {
      cpu->regs[stage->rd] = stage->buffer;
      {
        cpu->regs_valid[stage->rd] = 1;
      }
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
      print_stage_content("Execute 2 Stage", stage);
    }
  }

  return 0;
}

/*
 *  Memory 1 Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */

int memory1(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM1];
  if (!stage->busy && !stage->stalled)
  {

    /* Store */
    if (strcmp(stage->opcode, "STORE") == 0||
        strcmp(stage->opcode, "STR") == 0)
    {
      cpu->data_memory[stage->mem_address] = stage->rs1_value;
    }
      
    /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0 ||
        strcmp(stage->opcode, "LDR") == 0)
    {
      stage->buffer = cpu->data_memory[stage->mem_address];
    }

    /* Copy data from Memory 1 latch to Mmemory 2 latch*/
    if (!stage->stalled)
    {
      cpu->stage[MEM2] = cpu->stage[MEM1];
    }
    else
    {
      cpu->stage[EX2].stalled = 1;
      strcpy(cpu->stage[EX2].opcode, "EMPTY");
      //cpu->stage[EX2].pc = 0;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
      print_stage_content("Memory 1 Stage", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      //stage->stalled = 1;
      cpu->stage[EX2].stalled = 1;
      cpu->stage[EX2].pc = 0;
      cpu->stage[F].busy = 1;
    }
  }
  return 0;
}

/*
 *  Memory 2 Stage of APEX Pipeline
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int memory2(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM2];
  if (!stage->busy && !stage->stalled)
  {

    /* Copy data from Memory 2 latch to writeback latch*/
    if (!stage->stalled)
    {
      cpu->stage[WB] = cpu->stage[MEM2];
    }
    else
    {
      cpu->stage[MEM1].stalled = 1;
      strcpy(cpu->stage[WB].opcode, "EMPTY");
      //cpu->stage[WB].pc = 0;
    }
    /* Update register file */
    if (strcmp(stage->opcode, "LOAD") == 0 ||
        strcmp(stage->opcode, "LDR") == 0)
    {
      cpu->regs[stage->rd] = stage->buffer;
      {
        cpu->regs_valid[stage->rd] = 1;
      }
    }
    

    if (ENABLE_DEBUG_MESSAGES)
    {
      print_stage_content("Memory 2 Stage", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      //stage->stalled = 1;
      cpu->stage[MEM1].stalled = 1;
      cpu->stage[MEM1].pc = 0;
      cpu->stage[F].busy = 1;
    }
  }
  return 0;
}

/*
 *  Writeback Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int writeback(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[WB];
  if (!stage->busy && !stage->stalled)
  {
    if (strcmp(stage->opcode, "HALT") == 0)
      {
        HALT_FLAG = 1;
        //free(cpu->code_memory);
        //cpu->ins_completed = cpu->code_memory_size;
        cpu->stage[MEM2].stalled = 1;
        cpu->stage[MEM2].pc = 0;
        cpu->stage[F].busy = 1;
      }

      /* Update register file */
      if (strcmp(stage->opcode, "MUL") == 0 ||
          strcmp(stage->opcode, "SUB") == 0 ||
          strcmp(stage->opcode, "ADD") == 0 ||
          strcmp(stage->opcode, "ADDL") == 0 ||
          strcmp(stage->opcode, "SUBL") == 0)
      {
        if (strcmp(cpu->stage[MEM1].opcode, "ADD") != 0 &&
            strcmp(cpu->stage[MEM1].opcode, "SUB") != 0 &&
            strcmp(cpu->stage[MEM1].opcode, "MUL") != 0 &&
            strcmp(cpu->stage[MEM2].opcode, "ADD") != 0 &&
            strcmp(cpu->stage[MEM2].opcode, "SUB") != 0 &&
            strcmp(cpu->stage[MEM2].opcode, "MUL") != 0 &&
            strcmp(cpu->stage[EX1].opcode, "ADD") != 0 &&
            strcmp(cpu->stage[EX1].opcode, "SUB") != 0 &&
            strcmp(cpu->stage[EX1].opcode, "MUL") != 0 &&
            strcmp(cpu->stage[EX2].opcode, "ADD") != 0 &&
            strcmp(cpu->stage[EX2].opcode, "SUB") != 0 &&
            strcmp(cpu->stage[EX2].opcode, "MUL") != 0)
        {
          if (stage->buffer == 0)
          {
            cpu->z_flag_set = 1;
          }
        }
      }

      cpu->ins_completed++;

      if (ENABLE_DEBUG_MESSAGES)
      {
        print_stage_content("Writeback Stage", stage);
      }
    
  }
  return 0;
}

/*
 *  APEX CPU simulation loop
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int APEX_cpu_run(APEX_CPU *cpu)
{
  while (1)
  {

    /* All the instructions committed, so exit */
    if (cpu->ins_completed == cpu->code_memory_size || cpu->clock == cpu->clockcycles)
    {
      printf("(apex) >> Simulation Complete");
      break;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
      printf("--------------------------------\n");
      printf("Clock Cycle #: %d\n", ++cpu->clock);
      printf("--------------------------------\n");
    }

    writeback(cpu);
    memory2(cpu);
    memory1(cpu);
    execute2(cpu);
    execute1(cpu);
    decode(cpu);
    fetch(cpu);
    if (HALT_FLAG == 1)
    {
      break;
    }
  }

if (DISPLAY)
{
  display(cpu);
}
  return 0;
}