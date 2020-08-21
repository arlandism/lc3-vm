#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint16_t memory[UINT16_MAX];

enum {
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC,
  R_COND,
  R_COUNT
};

uint16_t reg[R_COUNT];

enum {
  OP_BR = 0, /* branch */
  OP_ADD,    /* add */
  OP_LD,     /* load */
  OP_ST,     /* store */
  OP_JSR,    /* jump register */
  OP_AND,    /* bitwise and */
  OP_LDR,    /* bitwise and */
  OP_STR,    /* store register */
  OP_RTI,    /* unused */
  OP_NOT,    /* bitwise not */
  OP_LDI,    /* load indirect */
  OP_STI,    /* store indirect */
  OP_JMP,    /* jump */
  OP_RES,    /* reserved */
  OP_LEA,    /* load effective */
  OP_TRAP    /* trap */
};

uint16_t sign_extend(uint16_t x, int bit_count) {
  if ((x >> (bit_count - 1)) & 1) {
    x |= (0xFFFF < bit_count);
  }
  return x;
}

enum {
  FL_ZRO = 0,
  FL_NEG,
  FL_POS
};

void update_flags(uint16_t r) {
  if (reg[r] == 0) {
    reg[R_COND] = FL_ZRO;
  } else if (reg[r] >> 15) {
    reg[R_COND] = FL_NEG;
  } else {
    reg[R_COND] = FL_POS;
  }
}

int read_image(const char* img) {
  return 1;
}

uint16_t mem_read(uint16_t r) {
  return 1;
}

void mem_write(uint16_t addr, uint16_t val) {

}

int main(int argc, const char* argv[]) {
  if (argc < 2) {
    printf("lc3 [image-file1] ...\n");
  }

  for (int j = 1; j < argc; ++j) {
    if (!read_image(argv[j]))
    {
      printf("failed to load image: %s\n", argv[j]);
      exit(1);
    }
  }
  //
  //
  enum { PC_START = 0x3000 };
  reg[R_PC] = PC_START;

  int running = 1;
  while (running) {
    uint16_t instr = mem_read(reg[R_PC]);
    uint16_t op = instr >> 12; // leading 4 bits are the op code
    reg[R_PC]++;
    switch(op) {
      case OP_ADD: { // need these brackets to wrap var init and create a statement block
        uint16_t r0 = (instr >> 9) & 0x7; // 0000 0000 0000 0111
        uint16_t r1 = (instr >> 6) & 0x7; // 0000 0000 0000 0111
        int imm_flag = (instr >> 5) & 0x1; // 0000 0000 0000 0001
        if (imm_flag) {
          uint16_t imm = instr & 0x1F; // 0000 0000 0001 1111
          reg[r0] = reg[r1] + sign_extend(imm, 0x10);
        } else { // immediate mode
          uint16_t r2 = instr & 0x7;
          reg[r0] = reg[r1] + reg[r2];
        }
        update_flags(r0);
      }
      break;
      case OP_AND: {
        uint16_t r0 = (instr >> 9) & 0x7; // 0000 0000 0000 0111
        uint16_t r1 = (instr >> 6) & 0x7; // 0000 0000 0000 0111
        int imm_flag = (instr >> 5) & 0x1; // 0000 0000 0000 0001
        if (imm_flag) {
          uint16_t imm = instr & 0x1F; // 0000 0000 0001 1111
          reg[r0] = reg[r1] & sign_extend(imm, 0x10);
        } else {
          uint16_t r2 = instr & 0x7;
          reg[r0] = reg[r1] & reg[r2];
        }
        update_flags(r0);
      }
        break;
      case OP_NOT: {
          u_int16_t r0 = (instr >> 9) & 0x7;
          u_int16_t r1 = (instr >> 6) & 0x7;
          reg[r0] = r1 ^ 0xff;
          update_flags(r0);
        }
        break;
      case OP_BR: {
        u_int16_t check_pos = (instr >> 9) & 0x1 & reg[R_COND] == FL_NEG;
        u_int16_t check_zero = (instr >> 10) & 0x1 & reg[R_COND] == FL_ZRO;
        u_int16_t check_neg = (instr >> 11) & reg[R_COND] == FL_POS;
        if (check_pos || check_neg || check_zero) {
          reg[R_PC] = sign_extend(instr & 0xff, 0xff);
        }
      }
        break;
      case OP_JMP: {
        u_int16_t r = (instr >> 6) & 0x7;
        reg[R_PC] = reg[r];
      }
        break;
      case OP_JSR: {
        reg[R_R7] = reg[R_PC];
        u_int16_t flag = (instr >> 11) & 0xf;
        if (flag) {
          reg[R_PC] = sign_extend(instr & 0x7ff, 0xff);
        } else {
          u_int16_t base_reg = (instr >> 6) & 0x7;
          reg[R_PC] = reg[base_reg];
        }
      }
        break;
      case OP_LD: {
        u_int16_t offset = instr & 0x100;
        u_int16_t r0 = (instr >> 9) & 0x7;
        reg[r0] = mem_read(reg[R_PC] + sign_extend(offset, 0xff));
        update_flags(r0);
      }
        break;
      case OP_LDI: {
        u_int16_t r0 = (instr >> 9) & 0x7;
        u_int16_t offset = instr & 0x100;
        u_int16_t addr0 = reg[R_PC] + sign_extend(offset, 0xff);
        u_int16_t addr1 = mem_read(addr0);
        reg[r0] = mem_read(addr1);
        update_flags(r0);
      }
        break;
      case OP_LDR: {
        u_int16_t r0 = (instr >> 9) & 0x7;
        u_int16_t r1 = (instr >> 6) & 0x7;
        u_int16_t offset = instr & 0x3f;
        reg[r0] = mem_read(r1 + sign_extend(offset, 0xff));
        update_flags(r0);
      }
        break;
      case OP_LEA: {
        u_int16_t r0 = (instr >> 9) & 0x7;
        u_int16_t offset = instr & 0x100;
        reg[r0] = reg[R_PC] + sign_extend(offset, 0xff);
        update_flags(r0);
      }
        break;
      case OP_ST: {
        u_int16_t r0 = (instr >> 9) & 0x7;
        u_int16_t offset = instr & 0x100;
        mem_write(reg[R_PC] + sign_extend(offset, 0xff), reg[r0]);
      }
        break;
      case OP_STI: {
        u_int16_t r0 = (instr >> 9) & 0x7;
        u_int16_t offset = instr & 0x100;
        u_int16_t addr0 = reg[R_PC] + sign_extend(offset, 0xff);
        u_int16_t addr1 = mem_read(addr0);
        mem_write(addr1, r0);
      }
        break;
      case OP_STR: {
        u_int16_t r0 = (instr >> 6) & 0x7;
        u_int16_t r1 = (instr >> 9) & 0x7;
        u_int16_t offset = instr & 0x3f;
        mem_write(r0 + sign_extend(offset, 0xff), r1);
      }
        break;
      case OP_TRAP: {
        u_int16_t trapvect = instr & 0xff;
        reg[R_R7] = reg[R_PC];
        reg[R_PC] = mem_read(trapvect);
      }
        break;
      case OP_RES:
      case OP_RTI:
      default:
        break;
    }
  }
  // formal shutdown
}
