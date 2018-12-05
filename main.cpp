#include <stdio.h>
#include "structures.h"

int memory[MEM_LEN];

// Globals initilazation
int nr_units_array[UNIT_TYPE_NUM];
int delays_array[UNIT_TYPE_NUM];

// current_cycle
functional_unit_t * fu_array_curr;
register_struct_t reg_file_curr[REG_NUM];
instruction_queue_t inst_queue_curr;

// next_cycle
functional_unit_t * fu_array_next;
register_struct_t reg_filey_next[REG_NUM];
instruction_queue_t inst_queuey_next;

int init_func() {
	init_registers();
	init_memory();
	init_global_params();  //arch spesifications
	init_functional_units();
	init_instruction_queue();
	return 0;
}

int is_end_program() {
	return 0;
}


int main() {

	init_func();

	while (!(is_end_program())) {
		backup_all_important_structs();  // copy next to curr
		fetch();
		issue();
		read_operants();
		execute();
		write_back();
		wrire_progress_to_output_files();
	}
	return 0;
}