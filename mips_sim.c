// COMP1521 20T3 Assignment 1: mips_sim -- a MIPS simulator
// starting point code v0.1 - 13/10/20

// PUT YOUR HEADER COMMENT HERE
// A program to simulate mips
// By Kellen Liew (z5312656)
// Finished 30/10/2020


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LENGTH 256
#define INSTRUCTIONS_GROW 64


// ADD YOUR #defines HERE


void execute_instructions(int n_instructions,
                          uint32_t instructions[n_instructions],
                          int trace_mode);
char *process_arguments(int argc, char *argv[], int *trace_mode);
uint32_t *read_instructions(char *filename, int *n_instructions_p);
uint32_t *instructions_realloc(uint32_t *instructions, int n_instructions);


// ADD YOUR FUNCTION PROTOTYPES HERE

// COMMAND FUNCTIONS, the following functions look for and
// handle their respective commands according to the spec. This
// includes flagging hex commands that are similar but otherwise
// invalid. Can also do -r and non -r modes

// A function to handle all syscall commands
int syscall(uint32_t instruction, int *terminals, int trace);

// A function to handle all add, sub and slt commands
int add_sub_slt(uint32_t instruction, uint32_t front, uint32_t back,
                                        int *terminals, int trace);

// A function to handle all mul commands
int mul(uint32_t instruction, uint32_t front, uint32_t back,
                                        int *terminals, int trace);

// A function to handle all addi, ori and lui commands
int addi_ori_lui(uint32_t instruction, uint32_t front,
                                        int *terminals, int trace);
// A function to handle all beq and bne commands
int beq_bne(uint32_t instruction, uint32_t front, int *pc,
                                        int *terminals, int trace);

// EXTRA HELPER FUNCTIONS, these are non-command functions that
// help other functions

// A function to convert unsigned (usually uint32_t) variables
// into signed variables. This allows for negative numbers to
// be represented
int convert_to_signed(uint32_t num);

// A function that prevents some constant terminals from
// being overwritten by commands. This makes the simulation
// consistent with how mips operates.
void consistency(int *terminals);

// YOU SHOULD NOT NEED TO CHANGE MAIN
// Note: Main is unchanged

int main(int argc, char *argv[]) {
    int trace_mode;
    char *filename = process_arguments(argc, argv, &trace_mode);

    int n_instructions;
    uint32_t *instructions = read_instructions(filename, &n_instructions);

    execute_instructions(n_instructions, instructions, trace_mode);

    free(instructions);
    return 0;
}


// simulate execution of  instruction codes in  instructions array
// output from syscall instruction & any error messages are printed
//
// if trace_mode != 0:
//     information is printed about each instruction as it executed
//
// execution stops if it reaches the end of the array

void execute_instructions(int n_instructions,
                          uint32_t instructions[n_instructions],
                          int trace_mode) {
    // REPLACE CODE BELOW WITH YOUR CODE

    int pc = 0;
    // Array meant to simulate the terminals
    int terminals[32] = {0};

    while (pc < n_instructions) {
        
        // Get the first and last 6 bits, these will help identify commands
        uint32_t front = instructions[pc] >> 26;
        uint32_t back  = (instructions[pc] & 0x3f);
        
        int *pc_point = &pc;
        if (trace_mode == 1) {
            printf("%d: 0x%08X ", pc, instructions[pc]);
        }
        
        // Execute command functions, only one should fire each loop
        if (add_sub_slt(instructions[pc], front, back, terminals, trace_mode) == 1) {
            return;
        }
        if (syscall(instructions[pc], terminals, trace_mode) == 1) {
            return;
        }
        if (mul(instructions[pc], front, back, terminals, trace_mode) == 1) {
            return;
        }
        if (addi_ori_lui(instructions[pc], front, terminals, trace_mode) == 1) {
            return;
        }
        if (beq_bne(instructions[pc], front, terminals, pc_point, trace_mode) == 1) {
            return;
        }
        
        // Ensure consistency of terminals
        consistency(terminals);

        pc++;
    }
    return;
}

//IDEA: Seperate functions for all instructions
// 1. Check for correct conditions, if so change telltale variable
// 2. Do necessary stuff (Use universal function?)
// 3. Otherwise move to next function, if telltale variable isn't triggered

// ADD YOUR FUNCTIONS HERE
// A function to handle all add, sub, slt commands
int add_sub_slt(uint32_t instruction, uint32_t front, uint32_t back,
                                            int *terminals, int trace) {
    // Check if conditions met (must be an add, sub or slt
    // instruction and NOT be a syscall)
    if (front != 0 || back == 0b001100) {
        return 0;
    }

    // Get s, t and d components
    uint32_t s = ((instruction >> 21) & 0x1f);
    uint32_t t = ((instruction >> 16) & 0x1f);
    uint32_t d = ((instruction >> 11) & 0x1f);
    
    // Ensure bits 21-26 are zeroes (valid code)
    uint32_t empty = ((instruction >> 6) & 0x1f);
    if (empty != 0) {
        printf("invalid instruction code\n");
            return 1;
    }

    // Depending on type of instruction, execute
    switch(back) {
        // Add
        case 0b100000 :
            if (trace == 1) {
                printf("add  $%d, $%d, $%d\n", d, s, t);
            }
            terminals[d] = terminals[s] + terminals[t];
            break;

        // Subtract
        case 0b100010 :
            if (trace == 1) {
                printf("sub  $%d, $%d, $%d\n", d, s, t);
            }
            terminals[d] = terminals[s] - terminals[t];
            break;
        
        // Set on less than
        case 0b101010 :
            if (trace == 1) {
                printf("slt  $%d, $%d, $%d\n", d, s, t);
            }

            if (terminals[s] < terminals[t]) {
                terminals[d] = 1;
            } else {
                terminals[d] = 0;
            }
            break;

        // Doesn't fullfil any cases, must be invalid code
        default :
            printf("invalid instruction code\n");
            return 1;
    }
    if (trace == 1) {
        printf(">>> $%d = %d\n", d, terminals[d]);
    }
    
    return 0;
}

// A function to handle all mul commands
int mul(uint32_t instruction, uint32_t front, uint32_t back, int *terminals,
                                                                int trace) {
    // Check if conditions met (front and back are specific values)
    if (front != 0b011100 || back != 0b00010) {
        return 0;
    }

    // Get s, t and d components
    uint32_t s = ((instruction >> 21) & 0x1f);
    uint32_t t = ((instruction >> 16) & 0x1f);
    uint32_t d = ((instruction >> 11) & 0x1f);

    // Ensure bits 21-26 are zeroes (valid code)
    uint32_t empty = ((instruction >> 6) & 0x1f);
    if (empty != 0) {
        printf("invalid instruction code\n");
            return 1;
    }

    // Follow on with mul command
    if (trace == 1) {
        printf("mul  $%d, $%d, $%d\n", d, s, t);
    }
    terminals[d] = terminals[s] * terminals[t];
    
    if (trace == 1) {
        printf(">>> $%d = %d\n", d, terminals[d]);
    }
    return 0;
}

// A function to handle all syscall commands
int syscall(uint32_t instruction, int *terminals, int trace) {
    
    // Check if conditions are met (instruction is a specific value)
    if (instruction != 0b1100) {
        return 0;
    }
    int v0 = terminals[2];
    int a0 = terminals[4];

    if (trace == 1) {
        printf("syscall\n");
        printf(">>> syscall %d\n", v0);
    }

    //Depending on what kind of instructions execute
    switch(v0) {
        // Print integer
        case 1 :
            if (trace == 1) {
                printf("<<< %d\n", a0);
            } else {
                printf("%d", a0);
            }
            break;

        // Exit
        case 10 :
            //printf("<<<\n");
            // Reset all terminals
            for (int j = 0; j < 32; j++) {
                terminals[j] = 0;
            }
            exit(0);
            break;

        // Print character
        case 11 :
            if (trace == 1) {
                printf("<<< %c\n", a0);
            } else {
                printf("%c", a0);
            }
            break;

        // Invalid syscall number provided
        default :
            printf("Unknown system call: %d\n", v0);
            return 1;
    }
    return 0;
}

// A function to handle all addi, ori and lui commands
int addi_ori_lui(uint32_t instruction, uint32_t front, int *terminals, int trace) {
    // Check if conditions are met (front has certain range)
    if (front < 0b001000 || front > 0b001111) {
        return 0;
    }

    // Get s,t and i
    uint32_t s = ((instruction >> 21) & 0x1f);
    uint32_t t = ((instruction >> 16) & 0x1f);
    uint32_t proto_i = (instruction & 0xffff);
    int i = convert_to_signed(proto_i);

    // Ensure bits 7-11 are zeroes for lui (valid code)
    int empty = ((instruction >> 21) & 0x1f);
    if (empty != 0 && front == 0b001111) {
        printf("invalid instruction code\n");
        return 1;
    }

    // Execute commands depending on instructions
    switch(front) {
        // Add integer
        case 0b001000 :
            if (trace == 1) {
                printf("addi $%d, $%d, %d\n", t, s, i);
            }
            terminals[t] = terminals[s] + i;
            break;

        // Or integer
        case 0b001101 :
            if (trace == 1) {
                printf("ori  $%d, $%d, %d\n", t, s, i);
            }
            terminals[t] = (terminals[s] | i);
            break;
        
        // Load upper immediate
        case 0b001111 :
            if (trace == 1) {
                printf("lui  $%d, %d\n", t, i);
            }
            terminals[t] = (i << 16);
            break;

        // No cases fulfilled, must be invalid
        default :
            printf("invalid instruction code\n");
    }

    if (trace == 1) {
        printf(">>> $%d = %d\n", t, terminals[t]);
    }
    return 0;
}

// A function to handle all beq and bne commands
int beq_bne(uint32_t instruction, uint32_t front, int *terminals, int *pc, int trace) {
    // Check if conditions fulfilled (check fronts)
    if (front != 0b000101 && front != 0b000100) {
        return 0;
    }

    // Get s, t and i
    uint32_t s = ((instruction >> 21) & 0x1f);
    uint32_t t = ((instruction >> 16) & 0x1f);
    uint32_t proto_i = (instruction & 0xffff);
    int i = convert_to_signed(proto_i);

    // Execute commands based on instructions
    switch(front) {
        // Branch on equal
        case 0b000100 :
            if (trace == 1) {
                printf("beq  $%d, $%d, %d\n", s, t, i);
            }

            if (terminals[s] == terminals[t]) {
                *pc += i;
                if (trace == 1) {
                    printf(">>> branch taken to PC = %d\n", *pc);
                }
                
                // Account for the PC uptick in the execute function loop
                *pc -= 1;
                
            } else {
                if (trace == 1) {
                    printf(">>> branch not taken\n");
                }
            }
            break;

        // Branch on not equal
        case 0b000101 :
            if (trace == 1) {
                printf("bne  $%d, $%d, %d\n", s, t, i);
            }
            if (terminals[s] != terminals[t]) {
                *pc += i;
                if (trace == 1) {
                    printf(">>> branch taken to PC = %d\n", *pc);
                }
                // Account for the PC uptick in the execute function loop
                *pc -= 1;
            
            } else {
                if (trace == 1) {
                    printf(">>> branch not taken\n");
                }
            }
            break;
    }

    // Make sure valid PC is given
    if (*pc <= -1) {
        printf("Illegal branch to address before instructions: PC = %d\n", (*pc + 1));
        return 1;
    }
    return 0;
}

// Convert unsigned variables to signed
int convert_to_signed(uint32_t num) {
    if (num >= 32768) {
        return num -= 65536;
    } else {
        return num;
    }
}

// A function to preserve consistency of terminals
// If a constant terminal is changed, it gets changed back
void consistency(int *terminals) {
    terminals[0]  = 0;
    terminals[26] = 0;
    terminals[27] = 0;
}

// YOU DO NOT NEED TO CHANGE CODE BELOW HERE


// check_arguments is given command-line arguments
// it sets *trace_mode to 0 if -r is specified
//          *trace_mode is set to 1 otherwise
// the filename specified in command-line arguments is returned

char *process_arguments(int argc, char *argv[], int *trace_mode) {
    if (
        argc < 2 ||
        argc > 3 ||
        (argc == 2 && strcmp(argv[1], "-r") == 0) ||
        (argc == 3 && strcmp(argv[1], "-r") != 0)) {
        fprintf(stderr, "Usage: %s [-r] <file>\n", argv[0]);
        exit(1);
    }
    *trace_mode = (argc == 2);
    return argv[argc - 1];
}


// read hexadecimal numbers from filename one per line
// numbers are return in a malloc'ed array
// *n_instructions is set to size of the array

uint32_t *read_instructions(char *filename, int *n_instructions_p) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "%s: '%s'\n", strerror(errno), filename);
        exit(1);
    }

    uint32_t *instructions = NULL;
    int n_instructions = 0;
    char line[MAX_LINE_LENGTH + 1];
    while (fgets(line, sizeof line, f) != NULL) {

        // grow instructions array in steps of INSTRUCTIONS_GROW elements
        if (n_instructions % INSTRUCTIONS_GROW == 0) {
            instructions = instructions_realloc(instructions, n_instructions + INSTRUCTIONS_GROW);
        }

        char *endptr;
        instructions[n_instructions] = strtol(line, &endptr, 16);
        if (*endptr != '\n' && *endptr != '\r' && *endptr != '\0') {
            fprintf(stderr, "%s:line %d: invalid hexadecimal number: %s",
                    filename, n_instructions + 1, line);
            exit(1);
        }
        n_instructions++;
    }
    fclose(f);
    *n_instructions_p = n_instructions;
    // shrink instructions array to correct size
    instructions = instructions_realloc(instructions, n_instructions);
    return instructions;
}


// instructions_realloc is wrapper for realloc
// it calls realloc to grow/shrink the instructions array
// to the speicfied size
// it exits if realloc fails
// otherwise it returns the new instructions array
uint32_t *instructions_realloc(uint32_t *instructions, int n_instructions) {
    instructions = realloc(instructions, n_instructions * sizeof *instructions);
    if (instructions == NULL) {
        fprintf(stderr, "out of memory");
        exit(1);
    }
    return instructions;
}