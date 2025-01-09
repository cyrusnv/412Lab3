/*
 * Honestly guys? I'm including this because it's what I included in a
 * 321 project, and I want to be sure I'm doing things right here.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "ir.h"

// Function prototypes; you know the vibe.
void finish_memop(int lexeme);
void finish_loadI(int lexeme);
void finish_arithop(int lexeme);
void finish_output(int lexeme);
void finish_nop(int lexeme);
void printIR();
struct Instruction *bcmalloc();

// A bunch of definitions. Who knows where this will go.
#define MAXLINE 1024 // Maximum line size, which I made up
#define HELPMSG "Command Syntax:\n"                                                        \
                "    ./lab1_ref [flags] filename\n"                                        \
                "Required arguments:\n"                                                    \
                "    filename  is the pathname (absolute or relative) to the input file\n" \
                "Optional flags:\n"                                                        \
                "    -h       prints this message\n"                                       \
                "At most one of the following three flags:\n"                              \
                "    -s       prints tokens in token stream\n"                             \
                "    -p       invokes parser and reports on success or failure\n"          \
                "    -r       prints human readable version of parser's IR\n"              \
                "If none is specified, the default action is '-p'.\n"



// Let's define some structs for the IR, yeah?
/* For now, I'm not using operand. I think the numbers are easy enough.
struct Operand {
     int sr;
     int vr;
     int pr;
     int nu;
};
*/

// Global variables. I am so sorry, Dr. Rixner.
char *file_buffer;
size_t bufsize = MAXLINE * sizeof(char);
char *buffer_ptr;
FILE *input_file;
int fileline = 0;
bool at_EOF = false;
bool hadError = false;
int opcount = 0;
int maxregister = 0;
bool h_flag = 0;
bool r_flag = 0;
bool p_flag = 0;
bool s_flag = 0;

struct Instruction *i_start;
struct Instruction *i_end;

// These globals pertain to doing big reallocs
int expansion_count = 1;
struct Instruction *ir_start;
int ir_curr = 0;
struct Instruction *curr_rec;

// NOTE: Surely you're going to need to clear this...
// int pair[2] = { -2, -2 };
char cat_lex_list[25][10] = {
    "NOP",
    "OUTPUT",
    "INTO",
    "COMMA",
    "EOL",
    "MEMOP",
    "ARITHOP",
    "LOADI",
    "load",
    "store",
    "loadI",
    "add",
    "sub",
    "mult",
    "lshift",
    "rshift",
    "output",
    "nop",
    ",",
    "=>",
    "EOF",
    "CONSTANT",
    "REGISTER",
};

// A function that does getline a bit more safely
int Getline()
{
     // printf("Got past here!\n");
     int result = getline(&file_buffer, &bufsize, input_file);
     if (result == -1)
     {
          // printf("getline failure. indicates EOF.\n");
          at_EOF = true;
     }
     else
     {
          buffer_ptr = file_buffer;
          // printf("getline result: %s\n", buffer_ptr);
          fileline++;
     }
     return result;
}

// A function that does the bulk allocation when needed, or just returns a pointer
struct Instruction *
bcmalloc()
{
     // The pointer we're going to ultimately return, where the next record starts
     // If we're in the very first pass
     //printf("ir_curr: %d", ir_curr);
     if (ir_curr == 0)
     {
          // Allocate enough space for 2000 records
          ir_start = malloc(2000 * sizeof(struct Instruction));
          // Ensure we allocate more memory in the future
          curr_rec = ir_start;
          ir_curr++;
          return curr_rec;
     }
     else if (ir_curr == 2000)
     {

          // Reallocate with an expanded size
          struct Instruction *newptr = malloc(2000 * sizeof(struct Instruction));
          ir_curr = 0;
          curr_rec = newptr;
          ir_curr++;
          return curr_rec;
     }
     else
     {
          // Return the pointer to the current block, and iterate what the next
          // block will be.
          curr_rec++;
          ir_curr++;
          return curr_rec;
     }
}

// A function that scans a line from a file. What else do you want from me.
// We assume the size of tokarray is 2, because if it isn't ur a thick shite
void gettoken(int tokarray[])
{
     tokarray[0] = -2;
     tokarray[1] = -2;

     char *c_init = buffer_ptr;
     char *c = buffer_ptr;
     // printf("gettoken line (as of c): %s\n", c);

     // Start by consuming all of the whitespace that starts a line.
     // TODO: You might want this to consume the newline, actually. Sorry!
     while (isspace(*c) != 0 && *c != '\n')
     {
          c++;
     }
     // printf("Out the gate: %c\n", *c);

     /* Start checking for categories. I don't like this either. */

     if (*c == '/')
     {
          c++;
          if (*c == '/')
          {
               while (*c != '\n' && *c != '\0')
               {
                    c++;
                    // printf("the character: %s\n", c);
               }
               if (*c == '\n')
               { // EOL
                    // printf("I see the newline!\n");
                    tokarray[0] = EOL;
                    tokarray[1] = EOL;

                    // printf("post newline: %s", buffer_ptr);
               }
               else if (*c == '\0')
               { // EOF
                    tokarray[0] = EOFF;
                    tokarray[1] = EOFF;
               }
          }
     }
     else if (*c == 'n') // NOP (from video example)
     {
          c++;
          // printf("%c\n", *c);
          if (*c == 'o')
          {
               c++;
               if (*c == 'p')
               {
                    tokarray[0] = NOP;
                    tokarray[1] = NOPL;
               }
          }
     }
     else if (*c == 's')
     { // store or sub
          c++;
          if (*c == 'u')
          {
               c++;
               if (*c == 'b')
               {
                    tokarray[0] = ARITHOP;
                    tokarray[1] = SUB;
               }
          }
          else if (*c == 't')
          {
               c++;
               if (*c == 'o')
               {
                    c++;
                    if (*c == 'r')
                    {
                         c++;
                         if (*c == 'e')
                         {
                              tokarray[0] = MEMOP;
                              tokarray[1] = STORE;
                         }
                    }
               }
          }
     }
     else if (*c == 'l')
     { // load or loadi or lshift
          c++;
          if (*c == 'o')
          {
               c++;
               if (*c == 'a')
               {
                    c++;
                    if (*c == 'd')
                    {
                         c++;
                         if (*c == 'I')
                         {
                              tokarray[0] = LOADI;
                              tokarray[1] = LOADIL;
                              // YOU MIGHT NEED TO UN-CONSUME HERE
                         }
                         else
                         {
                              // THIS IS A CASE YOU NEED TO SORT OUT
                              c--;
                              tokarray[0] = MEMOP;
                              tokarray[1] = LOAD;
                         }
                    }
               }
          }
          else if (*c == 's')
          {
               c++;
               if (*c == 'h')
               {
                    c++;
                    if (*c == 'i')
                    {
                         c++;
                         if (*c == 'f')
                         {
                              c++;
                              if (*c == 't')
                              {
                                   tokarray[0] = ARITHOP;
                                   tokarray[1] = LSHIFT;
                              }
                         }
                    }
               }
          }
     }
     else if (*c == 'r')
     { // rshift
          c++;
          if (*c == 's')
          {
               c++;
               if (*c == 'h')
               {
                    c++;
                    if (*c == 'i')
                    {
                         c++;
                         if (*c == 'f')
                         {
                              c++;
                              if (*c == 't')
                              {
                                   tokarray[0] = ARITHOP;
                                   tokarray[1] = RSHIFT;
                              }
                         }
                    }
               }
          }
          else if ('0' <= *c && *c <= '9')
          {
               int t = *c - '0';
               int n = t;
               c++;
               while ('0' <= *c && *c <= '9')
               {
                    t = *c - '0';
                    n = (n * 10) + t;
                    c++;
               }
               tokarray[0] = REGISTER;
               tokarray[1] = n;
               if (n > maxregister) {
                    maxregister = n;
               }
          }
     }
     else if (*c == 'm')
     { // mult
          c++;
          if (*c == 'u')
          {
               c++;
               if (*c == 'l')
               {
                    c++;
                    if (*c == 't')
                    {
                         tokarray[0] = ARITHOP;
                         tokarray[1] = MULT;
                    }
               }
          }
     }
     else if (*c == 'a')
     { // add
          c++;
          if (*c == 'd')
          {
               c++;
               if (*c == 'd')
               {
                    tokarray[0] = ARITHOP;
                    tokarray[1] = ADD;
               }
          }
     }
     else if (*c == 'o')
     { // output
          c++;
          if (*c == 'u')
          {
               c++;
               if (*c == 't')
               {
                    c++;
                    if (*c == 'p')
                    {
                         c++;
                         if (*c == 'u')
                         {
                              c++;
                              if (*c == 't')
                              {
                                   tokarray[0] = OUTPUT;
                                   tokarray[1] = OUTPUTL;
                              }
                         }
                    }
               }
          }
     }
     else if (*c == '=')
     { // INTO
          c++;
          if (*c == '>')
          {
               tokarray[0] = INTO;
               tokarray[1] = INTOL;
          }
     }
     else if (*c == ',')
     { // COMMA
          tokarray[0] = COMMA;
          tokarray[1] = COMMAL;
     }
     else if (*c == '\n')
     { // EOL
          // printf("I see the newline!\n");
          tokarray[0] = EOL;
          tokarray[1] = EOL;

          // printf("post newline: %s", buffer_ptr);
     }
     else if (*c == '\0')
     { // EOF
          tokarray[0] = EOFF;
          tokarray[1] = EOFF;
     }
     else if ('0' <= *c && *c <= '9')
     { // CONSTANT
          int t = *c - '0';
          int n = t;
          c++;
          while ('0' <= *c && *c <= '9')
          {
               t = *c - '0';
               n = (n * 10) + t;
               c++;
          }
          tokarray[0] = CONSTANT;
          tokarray[1] = n;
     }

     if (tokarray[0] == -2 && tokarray[1] == -2)
     {
          while (isspace(*c) == 0)
          {
               c++;
          }
          c--;
          fprintf(stderr, "ERROR %d: %.*s is not a valid word.\n", fileline, (int)(c - c_init) + 1, c_init);
          hadError = true;
     }
     // This is to make sure that the next time gettoken is called we're on a new
     // character. This may need to be modified as we run into newlines etc.
     // printf("before the iteration: %c\n", *c);
     if (tokarray[0] != EOL && tokarray[0] != REGISTER && tokarray[0] != CONSTANT)
     {
          c++;
     }
     // printf("after the iteration: %c\n", *c);
     /*
     if (*c != '\n' && *c != '\0') {
          printf("c: %c\n", *c);
     } else if (*c == '\n') {
          printf("c: newline\n");
     } else if (*c == '\0') {
          printf("c: EOF\n");
     }
     */

     if (s_flag)
     {
          if (tokarray[0] == REGISTER)
          {
               printf("<%s, r%d>\n", cat_lex_list[tokarray[0]], tokarray[1]);
          }
          else if (tokarray[0] == CONSTANT)
          {
               printf("<%s, %d>\n", cat_lex_list[tokarray[0]], tokarray[1]);
          }
          else
          {
               printf("<%s, %s>\n", cat_lex_list[tokarray[0]], cat_lex_list[tokarray[1]]);
          }
     }

     buffer_ptr = c;
     // printf("%i", loopcount);
}

// Imagine a program that could parse. Welcome to that program
void parse()
{
     int word[2] = {-2, -2};
     gettoken(word);
     // printf("first token word, before while loop: %s\n", cat_lex_list[word[0]]);
     // printf("first token number, before while loop: %d\n", word[0]);

     // TODO: Handle case where the first token is something stupid like EOF.
     // for (int i = 0; i < 8; i++)
     while (word[0] != EOFF && at_EOF == false)
     {
          //printf("fileline: %d\n", fileline);
          // printf("Starting a parse loop.\n");
          // for (int i = 0; i < 5; i++) {
          // printf("Parse loop, iteration: %d\n", i);
          switch (word[0])
          {
          case MEMOP:
               // printf("We've found a confirmed MEMOP.\n");
               finish_memop(word[1]);
               break;

          case LOADI:
               // printf("We've got a LOADI\n");
               finish_loadI(word[1]);
               break;

          case ARITHOP:
               // printf("arithop happening!\n");
               finish_arithop(word[1]);
               break;

          case OUTPUT:
               // printf("Guess what? Output\n");
               finish_output(word[1]);
               break;

          case NOP:
               finish_nop(word[1]);
               break;

          case EOFF:
               // finish_EOF();
               break;

          case EOL:
               // finish_EOL(word[1]);
               Getline();
               break;

          // TODO: throw an error, gracefully.
          default:
               // printf("Default case triggered, no word found to start line.\n");
               Getline();
          }
          if (!at_EOF)
          {
               gettoken(word);
               // printf("the new word is: %s\n", cat_lex_list[word[0]]);
          }
     }
}

// This used to be running main. It's not anymore, obviously.
struct Instruction* buildIR(char* filename)
{
     /*
      * while (you're not at the end of the file) {
      *      parseline()
      * }
      *
      * */

     // Set up the doubly-linked, cicular list that will be your IR.
     i_start = bcmalloc();
     i_start->line = 0;
     i_end = bcmalloc();
     i_end->line = -1;
     i_start->next = i_end;
     i_start->prev = i_end;
     i_end->next = i_start;
     i_end->prev = i_start;

     // Pretty sure this isn't needed, but I'm not killing it just yet.
     //int fcount = 0;


     // char *input_filename = "./test_inputs/sample.txt";
     char *input_filename = filename;
     input_file = fopen(input_filename, "r");
     if (input_file == NULL)
     {
          fprintf(stderr, "ERROR: File not found.\n");
          hadError = true;
          exit(-1);
     }
     Getline();
     parse();

     // Do a stupid check to decrement the line of the last
     // value in the IR, because this is Bad Programming.
     if (i_start->next != i_end)
     {
          i_end->prev->line++;
     }

     return (i_start);

}

/*
 * Requires:
 *  instr -- A pointer to the instruction to be inserted into a linked list.
 *  target -- The pointer to the instruction after which instr will be inserted
 *
 * Effects:
 *  Inserts instr into a circular doubly linked list after target. Returns
 *  0 if successful.
 */
int insert_instr_after(struct Instruction *instr, struct Instruction *target)
{
     // The instruction currently following target.
     struct Instruction *third = target->next;
     third->prev = instr;
     instr->next = third;

     instr->prev = target;

     target->next = instr;

     // Return 0 upon completion.
     return (0);
}

void finish_memop(int lexeme)
{
     bool success = 0;
     int firstregister[2] = {-2, -2};
     gettoken(firstregister);
     if (firstregister[0] == REGISTER)
     {
          // printf("h1\n");
          int into[2] = {-2, -2};
          gettoken(into);
          if (into[0] == INTO)
          {
               // printf("h2\n");
               int secondregister[2] = {-2, -2};
               gettoken(secondregister);
               // printf("secondregister: %s\n", cat_lex_list[secondregister[0]]);
               if (secondregister[0] == REGISTER)
               {
                    // printf("h3\n");
                    int linedone[2] = {-2, -2};
                    gettoken(linedone);
                    // printf("linedone: %s\n", cat_lex_list[linedone[0]]);
                    if (linedone[0] == EOL || linedone[0] == EOFF)
                    {
                         if (linedone[0] == EOL)
                         {
                              // printf("h4\n");
                              Getline();
                              // printf("HELL YEAH\n");
                              success = 1;
                         }
                         else
                         {
                              success = 1;
                              // printf("HELL YEAH EOF\n");
                              at_EOF = true;
                         }

                         // printf("line: %d\n", i_start->line);
                         if (!hadError)
                         {
                              struct Instruction *newinstr;
                              newinstr = bcmalloc();
                              insert_instr_after(newinstr, i_end->prev);
                              int oldline = newinstr->prev->line;
                              oldline++;
                              // newinstr->line = oldline;
                              newinstr->line = fileline - 1;
                              // OPCODE AH SHIT
                              newinstr->opcode = lexeme;
                              // These have moved below a bit:
                              // newinstr->sr1 = firstregister[1];
                              // newinstr->sr3 = secondregister[1];

                              // All the other silly values to pre-fill:
                              newinstr->sr1 = firstregister[1];
                              newinstr->vr1 = -1;
                              newinstr->pr1 = -1;
                              newinstr->nu1 = -1;

                              newinstr->sr2 = -1;
                              newinstr->vr2 = -1;
                              newinstr->pr2 = -1;
                              newinstr->nu2 = -1;

                              newinstr->sr3 = secondregister[1];
                              newinstr->vr3 = -1;
                              newinstr->pr3 = -1;
                              newinstr->nu3 = -1;
                         }
                         opcount++;
                         /*
                         printf("**DOUBLE CHECKING NEWINSTR**\n");
                         printf("line: %d\n", newinstr->line);
                         printf("prev line: %d\n", newinstr->prev->line);
                         printf("next line: %d\n", newinstr->next->line);
                         printf("opcode: %s\n", cat_lex_list[newinstr->opcode]);
                         printf("**END CHECK**\n");
                         */
                    }
                    else
                    {
                         fprintf(stderr, "ERROR %d: Line did not end after second register.\n", fileline);
                         hadError = true;
                    }
               }
               else
               {
                    fprintf(stderr, "ERROR %d: Missing second register in instruction.\n", fileline);
                    hadError = true;
               }
          }
          else
          {
               fprintf(stderr, "ERROR %d: Missing \"=>\" in instruction.\n", fileline);
               hadError = true;
          }
     }
     else
     {
          fprintf(stderr, "ERROR %d: Missing first register in instruction.\n", fileline);
          hadError = true;
     }
     if (success == 0)
     {
          // printf("HELL NO MEMOP\n");
          Getline();
     }
}

void finish_loadI(int lexeme)
{
     // printf("loadI begun\n");
     bool success = 0;
     int constant[2] = {-2, -2};
     gettoken(constant);
     if (constant[0] == CONSTANT)
     {
          int into[2] = {-2, -2};
          gettoken(into);
          if (into[0] == INTO)
          {
               // printf("h2\n");
               int register_t[2] = {-2, -2};
               gettoken(register_t);
               // printf("register_t: %s\n", cat_lex_list[register_t[0]]);
               if (register_t[0] == REGISTER)
               {
                    int linedone[2] = {-2, -2};
                    gettoken(linedone);
                    // printf("linedone loadI: %d\n", linedone[0]);
                    if (linedone[0] == EOL || linedone[0] == EOFF)
                    {
                         if (linedone[0] == EOL)
                         {
                              // printf("h4\n");
                              //  TODO: You need a plan in the event that this returns EOF.
                              Getline();
                              // printf("HELL YEAH\n");
                              success = 1;
                         }
                         else
                         {
                              success = 1;
                              // printf("HELL YEAH EOF\n");
                              at_EOF = true;
                         }

                         // printf("line: %d\n", i_start->line);
                         if (!hadError)
                         {
                              struct Instruction *newinstr;
                              newinstr = bcmalloc();
                              insert_instr_after(newinstr, i_end->prev);
                              int oldline = newinstr->prev->line;
                              oldline++;
                              // newinstr->line = oldline;
                              newinstr->line = fileline - 1;
                              // OPCODE AH SHIT
                              newinstr->opcode = lexeme;
                              // These have moved down:
                              //newinstr->sr1 = constant[1];
                              //newinstr->sr3 = register_t[1];

                              // All the other silly values to pre-fill:
                              newinstr->sr1 = constant[1];
                              newinstr->vr1 = -1;
                              newinstr->pr1 = -1;
                              newinstr->nu1 = -1;

                              newinstr->sr2 = -1;
                              newinstr->vr2 = -1;
                              newinstr->pr2 = -1;
                              newinstr->nu2 = -1;

                              newinstr->sr3 = register_t[1];
                              newinstr->vr3 = -1;
                              newinstr->pr3 = -1;
                              newinstr->nu3 = -1;
                         }
                         opcount++;
                         /*
                         printf("**DOUBLE CHECKING NEWINSTR**\n");
                         printf("line: %d\n", newinstr->line);
                         printf("prev line: %d\n", newinstr->prev->line);
                         printf("next line: %d\n", newinstr->next->line);
                         printf("opcode: %s\n", cat_lex_list[newinstr->opcode]);
                         printf("**END CHECK**\n");
                         */
                    }
                    else
                    {
                         fprintf(stderr, "ERROR %d: Instruction has too many arguments.\n", fileline);
                         hadError = true;
                    }
               }
               else
               {
                    fprintf(stderr, "ERROR %d: Missing register in instruction\n", fileline);
                    hadError = true;
               }
          }
          else
          {
               fprintf(stderr, "ERROR %d: Missing \"=>\" in instruction\n", fileline);
               hadError = true;
          }
     }
     else
     {
          fprintf(stderr, "ERROR %d: Missing an initial constant in instruction\n", fileline);
          hadError = true;
     }
     if (success == 0)
     {
          // printf("HELL NO LOADI\n");
          Getline();
     }
}

void finish_arithop(int lexeme)
{
     bool success = 0;
     int reg1[2] = {-2, -2};
     gettoken(reg1);
     if (reg1[0] == REGISTER)
     {
          int comma[2] = {-2, -2};
          gettoken(comma);
          if (comma[0] == COMMA)
          {
               // printf("h2\n");
               int reg2[2] = {-2, -2};
               gettoken(reg2);
               // printf("reg2: %s\n", cat_lex_list[reg2[0]]);
               if (reg2[0] == REGISTER)
               {
                    int into[2] = {-2, -2};
                    gettoken(into);
                    if (into[0] == INTO)
                    {
                         int reg3[2] = {-2, -2};
                         // printf("All the way to into!\n");
                         gettoken(reg3);
                         // printf("%d %d\n", reg3[0], reg3[1]);
                         if (reg3[0] == REGISTER)
                         {
                              int linedone[2] = {-2, -2};
                              gettoken(linedone);
                              // printf("linedone: %s\n", cat_lex_list[linedone[0]]);
                              if (linedone[0] == EOL || linedone[0] == EOFF)
                              {
                                   if (linedone[0] == EOL)
                                   {
                                        // printf("h4\n");
                                        //  TODO: You need a plan in the event that this returns EOF.
                                        Getline();
                                        // printf("HELL YEAH\n");
                                        success = 1;
                                   }
                                   else
                                   {
                                        success = 1;
                                        // printf("HELL YEAH EOF\n");
                                        at_EOF = true;
                                   }

                                   // printf("line: %d\n", i_start->line);
                                   if (!hadError)
                                   {
                                        struct Instruction *newinstr;
                                        newinstr = bcmalloc();
                                        insert_instr_after(newinstr, i_end->prev);
                                        int oldline = newinstr->prev->line;
                                        oldline++;
                                        // newinstr->line = oldline;
                                        newinstr->line = fileline - 1;
                                        // OPCODE AH SHIT
                                        newinstr->opcode = lexeme;
                                        // These have moved down a few lines:
                                        // newinstr->sr1 = reg1[1];
                                        // newinstr->sr2 = reg2[1];
                                        // newinstr->sr3 = reg3[1];

                                        // All the other silly values to pre-fill:
                                        newinstr->sr1 = reg1[1];
                                        newinstr->vr1 = -1;
                                        newinstr->pr1 = -1;
                                        newinstr->nu1 = -1;

                                        newinstr->sr2 = reg2[1];
                                        newinstr->vr2 = -1;
                                        newinstr->pr2 = -1;
                                        newinstr->nu2 = -1;

                                        newinstr->sr3 = reg3[1];
                                        newinstr->vr3 = -1;
                                        newinstr->pr3 = -1;
                                        newinstr->nu3 = -1;
                                   }
                                   opcount++;
                                   /*
                                   printf("**DOUBLE CHECKING NEWINSTR**\n");
                                   printf("line: %d\n", newinstr->line);
                                   printf("prev line: %d\n", newinstr->prev->line);
                                   printf("next line: %d\n", newinstr->next->line);
                                   printf("opcode: %s\n", cat_lex_list[newinstr->opcode]);
                                   printf("**END CHECK**\n");
                                   */
                              }
                              else
                              {
                                   fprintf(stderr, "ERROR %d: Instruction has too many arguments.\n", fileline);
                                   hadError = true;
                              }
                         }
                         else
                         {
                              fprintf(stderr, "ERROR %d: Missing third register in instruction\n", fileline);
                              hadError = true;
                         }
                    }
                    else
                    {
                         fprintf(stderr, "ERROR %d: Missing \"=>\" in instruction\n", fileline);
                         hadError = true;
                    }
               }
               else
               {
                    fprintf(stderr, "ERROR %d: Missing register in instruction\n", fileline);
                    hadError = true;
               }
          }
          else
          {
               fprintf(stderr, "ERROR %d: Missing \",\" in instruction\n", fileline);
               hadError = true;
          }
     }
     else
     {
          fprintf(stderr, "ERROR %d: Missing an initial constant in instruction\n", fileline);
          hadError = true;
     }
     if (success == 0)
     {
          // printf("HELL NO ARITHOP\n");
          Getline();
     }
}

void finish_output(int lexeme)
{
     bool success = 0;
     int constant[2] = {-2, -2};
     gettoken(constant);
     if (constant[0] == CONSTANT)
     {
          int linedone[2] = {-2, -2};
          gettoken(linedone);
          // printf("I at least saw the constant.\n");
          // printf("linedone: %s\n", cat_lex_list[linedone[0]]);
          if (linedone[0] == EOL || linedone[0] == EOFF)
          {
               if (linedone[0] == EOL)
               {
                    // printf("h4\n");
                    //  TODO: You need a plan in the event that this returns EOF.
                    Getline();
                    // printf("HELL YEAH\n");
                    success = 1;
               }
               else
               {
                    success = 1;
                    // printf("HELL YEAH EOF\n");
                    at_EOF = true;
               }

               // printf("line: %d\n", i_start->line);
               if (!hadError)
               {
                    struct Instruction *newinstr;
                    newinstr = bcmalloc();
                    insert_instr_after(newinstr, i_end->prev);
                    int oldline = newinstr->prev->line;
                    oldline++;
                    // newinstr->line = oldline;
                    newinstr->line = fileline - 1;
                    // OPCODE AH SHIT
                    newinstr->opcode = lexeme;
                    // Moved a bit down...
                    // newinstr->sr1 = constant[1];

                    // All the other silly values to pre-fill:
                    newinstr->sr1 = constant[1];
                    newinstr->vr1 = -1;
                    newinstr->pr1 = -1;
                    newinstr->nu1 = -1;

                    newinstr->sr2 = -1;
                    newinstr->vr2 = -1;
                    newinstr->pr2 = -1;
                    newinstr->nu2 = -1;

                    newinstr->sr3 = -1;
                    newinstr->vr3 = -1;
                    newinstr->pr3 = -1;
                    newinstr->nu3 = -1;
               }
               opcount++;
               // printf("**DOUBLE CHECKING NEWINSTR**\n");
               // printf("line: %d\n", newinstr->line);
               // printf("prev line: %d\n", newinstr->prev->line);
               // printf("next line: %d\n", newinstr->next->line);
               // printf("opcode: %s\n", cat_lex_list[newinstr->opcode]);
               // printf("**END CHECK**\n");
          }
          else
          {
               fprintf(stderr, "ERROR %d: Instruction has too many arguments.\n", fileline);
               hadError = true;
          }
     }
     else
     {
          fprintf(stderr, "ERROR %d: Missing an initial constant in instruction\n", fileline);
          hadError = true;
     }
     if (success == 0)
     {
          // printf("HELL NO LOADI\n");
          Getline();
     }
}

void finish_nop(int lexeme)
{
     int success = 1;
     int linedone[2] = {-2, -2};
     gettoken(linedone);
     // printf("linedone: %s\n", cat_lex_list[linedone[0]]);
     if (linedone[0] == EOL || linedone[0] == EOFF)
     {
          if (linedone[0] == EOL)
          {
               // printf("h4\n");
               //  TODO: You need a plan in the event that this returns EOF.
               Getline();
               // printf("HELL YEAH\n");
               success = 1;
          }
          else
          {
               success = 1;
               // printf("HELL YEAH EOF\n");
               at_EOF = true;
          }

          // printf("line: %d\n", i_start->line);
          if (!hadError)
          {
               struct Instruction *newinstr;
               newinstr = bcmalloc();
               insert_instr_after(newinstr, i_end->prev);
               int oldline = newinstr->prev->line;
               oldline++;
               // newinstr->line = oldline;
               newinstr->line = fileline - 1;
               // OPCODE AH SHIT
               newinstr->opcode = lexeme;
          }
          opcount++;
          // newinstr->sr1 = constant[1];

          // printf("**DOUBLE CHECKING NEWINSTR**\n");
          // printf("line: %d\n", newinstr->line);
          // printf("prev line: %d\n", newinstr->prev->line);
          // printf("next line: %d\n", newinstr->next->line);
          // printf("opcode: %s\n", cat_lex_list[newinstr->opcode]);
          // printf("**END CHECK**\n");
     }
     else
     {
          fprintf(stderr, "ERROR %d: Instruction has too many arguments.\n", fileline);
          hadError = true;
     }
     if (success == 0)
     {
          // printf("HELL NO LOADI\n");
          Getline();
     }
}

void printIR()
{
     printf("**PRINT IR**\n");
     struct Instruction *currInstr = i_start;
     printf("starting line number: %d\n", currInstr->line);
     printf("(To be clear: line 0 is a dummy head, not an actual line.)\n");

     while (currInstr->line != -1)
     {

          printf("LINE: %d\n", currInstr->line);
          printf("  opcode: %s\n", cat_lex_list[currInstr->opcode]);
          printf("  sr1: %d\n", currInstr->sr1);
          printf("  sr2: %d\n", currInstr->sr2);
          printf("  sr3: %d\n", currInstr->sr3);
          printf("\n");

          currInstr = currInstr->next;
     }
}

void printIR2(struct Instruction* ir)
{
     printf("**PRINT IR**\n");
     struct Instruction *currInstr = ir;
     printf("starting line number: %d\n", currInstr->line);
     printf("(To be clear: line 0 is a dummy head, not an actual line.)\n");

     while (currInstr->line != -1)
     {

          printf("LINE: %d\n", currInstr->line);
          printf("  opcode: %s\n", cat_lex_list[currInstr->opcode]);
          printf("  arg1:");
          printf("       sr1: %d ", currInstr->sr1);
          printf("       vr1: %d ", currInstr->vr1);
          printf("       pr1: %d ", currInstr->pr1);
          printf("       nu1: %d\n", currInstr->nu1);
          printf("  arg2:");
          printf("       sr2: %d ", currInstr->sr2);
          printf("       vr2: %d ", currInstr->vr2);
          printf("       pr2: %d ", currInstr->pr2);
          printf("       nu2: %d\n", currInstr->nu2);
          printf("  arg3:");
          printf("       sr3: %d ", currInstr->sr3);
          printf("       vr3: %d ", currInstr->vr3);
          printf("       pr3: %d ", currInstr->pr3);
          printf("       nu3: %d ", currInstr->nu3);
          printf("\n");

          currInstr = currInstr->next;
     }
}