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
	
	if (argc != 7) {
		printf("Usage : sym.exe cfg.txt memin.txt memout.txt regout.txt traceinst.txt traceunit.txt \n");
		return -1;
	}

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
	//init_func();

	//while (!(is_end_program())) {
	//	backup_all_important_structs();  // copy next to curr
	//	fetch();
	//	issue();
	//	read_operants();
	//	execute();
	//	write_back();
	//	wrire_progress_to_output_files();
	//}
	return 0;
}