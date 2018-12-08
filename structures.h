#ifndef STRUCTURES_H_   /* Include guard */
#define STRUCTURES_H_
#include <stdio.h>
#include <stdlib.h>
#define INST_Q_LEN (16)
#define MEM_LEN		(4096)
#define UNIT_TYPE_NUM		(6)
#define REG_NUM		(16)
#define MAX_LINE_LEN (1000)



typedef enum op_code {
	LD = 0,
	ST,
	ADD,
	SUB,
	MULT,
	DIV,
	HALT,
}op_code_t;

typedef enum queue_command {
	POP = 0,
	PEEk
} queue_command_t;

typedef struct inst_struct{
	int src_reg_1;
	int src_reg_2;
	int dest_reg;
	op_code_t op_code;
	int immidiate;
}inst_struct_t;

typedef struct trace_unit_cfg {
	op_code_t unit_type;
	int unit_num;
} trace_unit_cfg_t;

typedef struct functional_unit {
	bool is_busy;
	op_code_t unit_type;
	int unit_index;
	bool is_trace;
	unsigned int time_left;
	bool is_waiting_for_writeback;
	int instruction_num;  // in memory
	int Fi;
	int Fj;
	int Fk;
	struct functional_unit * Qj;
	struct functional_unit * Qk;
	bool Rj;
	bool Rk;	
}functional_unit_t;

typedef struct register_struct {
	float value;
	bool is_ready;
	int func_unit_num;
}register_struct_t;

// Need to implement Q functionality
typedef struct instruction_queue{
	inst_struct_t inst_array[INST_Q_LEN];
	int read_ptr;
	int write_ptr;
	int free_spots;
}instruction_queue_t;

bool queue_is_free(instruction_queue_t * q);
int queue_push(instruction_queue_t * q, inst_struct_t * inst);
inst_struct_t * queue_read(instruction_queue_t * q, queue_command_t q_cmd);
void queue_print(instruction_queue_t * q);
void inst_print(inst_struct_t * inst);

#endif // STRUCTURES_H_