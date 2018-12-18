#include <stdio.h>
#include "scoreboard.h"

//int init_func() {
//	init_registers();
//	init_memory();
//	init_global_params();  //arch spesifications
//	init_functional_units();
//	init_instruction_queue();
//	return 0;
//}

int is_end_program() {
	return 0;
}


int main(int argc, char * argv[]) {
	for (int i = 0; i < argc; i++)
	{
		printf("argv[%d] = %s\n", i, argv[i]);
	}
	const char * cfg_path = argv[1];
	const char * memin_path = argv[2];
	const char * memout_path = argv[3];
	const char * regout_path = argv[4];
	const char * traceinst_path = argv[5];
	const char * traceunit_path = argv[6];
	init_func(cfg_path, memin_path, memout_path, regout_path, traceinst_path, traceunit_path);
	int cycle = 0;
	while(!is_stop_running())
	{ // TODO - need to handle HALT and end of execution		
		printf("cycle = %d\n", cycle);
		fetch();
		issue();
		read_operands();
		execute();
		write_back();		
		//print_regout(true);
		traceunit();
		sample_state();  // copy next to curr
		cycle++;
	}
	print_regout(false);
	print_memout(false);
	print_traceinst(false);
	return 0;
}