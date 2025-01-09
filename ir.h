// ir.h
#ifndef IR_H
#define IR_H

#include <stdbool.h>
#include <stdint.h>

// Definitions for opcodes
// Parts of speech
#define NOP 0
#define OUTPUT 1
#define INTO 2
#define COMMA 3
#define EOL 4
#define MEMOP 5
#define ARITHOP 6
#define LOADI 7
#define LOAD 8
#define STORE 9
#define LOADIL 10
#define ADD 11
#define SUB 12
#define MULT 13
#define LSHIFT 14
#define RSHIFT 15
#define OUTPUTL 16
#define NOPL 17
#define COMMAL 18
#define INTOL 19
#define EOFF 20
#define CONSTANT 21
#define REGISTER 22

// Global structs
struct Instruction {
    struct Instruction *next;
    struct Instruction *prev;
    int line;
    int opcode;
    int sr1, vr1, pr1, nu1;
    int sr2, vr2, pr2, nu2;
    int sr3, vr3, pr3, nu3;
};


//Global Variables
extern int opcount;
extern int maxregister;
extern char cat_lex_list[25][10];

// Main functions for Lab 2 to use
struct Instruction* buildIR(char* filename);
//void printIR(struct Instruction* ir);
struct Instruction* bcmalloc(void);
void printIR2(struct Instruction* ir);
int insert_instr_after(struct Instruction *instr, struct Instruction *target);


#endif