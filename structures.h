#ifndef STRUCTURES_H_   /* Include guard */
#define STRUCTURES_H_
#include <stdio.h>
#include <stdlib.h>
#define INST_Q_LEN (16)
#define MEM_LEN		(4096)
#define UNIT_TYPE_NUM		(6)
#define REG_NUM		(16)
#define MAX_LINE_LEN (1000)

// All possbile phases for instructions
typedef enum trace_inst_phase {
	ISSUE = 0,
	READ_OPERANDS,
	EXEC,
	WB
} trace_inst_phase_t;

// All supported opcodes
typedef enum op_code {
	LD = 0,
	ST,
	ADD,
	SUB,
	MULT,
	DIV,
	HALT,
}op_code_t;

// Possible commands for instruction queue
typedef enum queue_command {
	POP = 0,
	PEEK
} queue_command_t;

// All relevant information for tracing instructions
typedef struct traceinst
{
	int command_hex;
	int pc;
	op_code_t fu_type;
	int fu_idx;
	int issued;
	int read_operands;
	int exec;
	int wb;
} traceinst_t;

// Parsed instruction
typedef struct inst_struct{
	int instruction;
	int src_reg_1;
	int src_reg_2;
	int dest_reg;
	op_code_t op_code;
	int immidiate;
	int pc;
}inst_struct_t;

// Information about which unit to trace, derived from the cfg.txt file
typedef struct trace_unit_cfg {
	op_code_t unit_type;
	int unit_num;
} trace_unit_cfg_t;


// All relevant information for a FU
typedef struct functional_unit {
	bool is_busy; // set when fu has an issued command
	bool is_execute; // set when fu has started execution
	bool is_writeback; // set when fu execution is done
	op_code_t unit_type;
	int unit_index;
	bool is_trace;
	unsigned int time_left;	
	int instruction_num;  // in memory
	int Fi;
	int Fj;
	int Fk;
	struct functional_unit * Qj;
	struct functional_unit * Qk;
	bool Rj;
	bool Rk;	
	int immediate;
	float wb_val;
	int instruction;

}functional_unit_t;

// Single register
typedef struct register_struct {
	float value;
	bool is_ready;
	functional_unit_t * fu;
}register_struct_t;

// Instruction queue implemented as an array with pointers
typedef struct instruction_queue{
	inst_struct_t inst_array[INST_Q_LEN];
	int read_ptr;
	int write_ptr;
	int free_spots;
}instruction_queue_t;
// returns true if the instruction queue is empty, false otherwise
bool queue_is_empty(instruction_queue_t * q);
// returns true if the instruction queue has an available slot, false otherwise
bool queue_is_free(instruction_queue_t * q);
// pushed a given instruction to the next available spot in the queue
int queue_push(instruction_queue_t * q, inst_struct_t * inst);
// Pops/Peeks at the next instruction in the queue, according to q_cmd
inst_struct_t * queue_read(instruction_queue_t * q, queue_command_t q_cmd);
// debug information about the instruction queue
void queue_print(instruction_queue_t * q);
// prints decoded insturction
void inst_print(inst_struct_t * inst);
// prints field of fu struct
void fu_print(functional_unit_t * fu);
#endif // STRUCTURES_H_