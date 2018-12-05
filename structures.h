#ifndef STRUCTURES_H_   /* Include guard */
#define STRUCTURES_H_

#define INST_Q_LEN (16)
#define MEM_LEN		(4096)
#define UNIT_TYPE_NUM		(6)
#define REG_NUM		(16)




typedef enum op_code {
	LD = 0,
	ST,
	ADD,
	SUB,
	MULT,
	DIV,
	HALT,
}op_code_t;

typedef struct inst_struct{
	int src_reg_1;
	int src_reg_2;
	int dest_reg;
	op_code_t op_code;
	int immidiate;
}inst_struct_t;




typedef struct functional_unit {
	int is_busy;
	int unit_type;
	int time_left;
	int is_waiting_for_writeback;
	int instruction_num;  // in memory
	int Fi;
	int Fj;
	int Fk;
	int Qj;
	int Qk;
	int Rj;
	int Rk;	
}functional_unit_t;

typedef struct register_struct {
	int value;
	int is_ready;
	int func_unit_num;
}register_struct_t;

// Need to implement Q functionality
typedef struct instruction_queue{
	struct inst_struct_t inst_array[INST_Q_LEN];
	int read_ptr;
	int write_ptr;
	int free_spots;
}instruction_queue_t;

// push pop pick (look at the next inst)

#endif // STRUCTURES_H_