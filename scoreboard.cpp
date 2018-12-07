#include "scoreboard.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
int memory[MEM_LEN];

// Globals initilazation
int nr_units_array[UNIT_TYPE_NUM];
int delays_array[UNIT_TYPE_NUM];

trace_unit_cfg_t trace_unit_cfg;

// current_cycle
functional_unit_t ** fu_array_curr;
register_struct_t reg_file_curr[REG_NUM];
instruction_queue_t inst_queue_curr;

// next_cycle
functional_unit_t ** fu_array_next;
register_struct_t reg_filey_next[REG_NUM];
instruction_queue_t inst_queue_next;

int init_registers(register_struct_t * reg_array)
{
	for (int i = 0; i < REG_NUM; i++)
	{
		reg_array[i].value = float(i);
		reg_array[i].is_ready = true;
		reg_array[i].func_unit_num = 0;
	}
	return 1;
}

int init_memory(const char * memin_path)
{	
	FILE * mem_file = fopen(memin_path, "r");
	if (mem_file)
	{
		int row_num = 0;
		while (fscanf(mem_file, "%x", &memory[row_num]) == 1)
		{
			row_num++;
		}
	}
	return 1;
}

op_code_t get_unit_type_from_cfg_line(char * line)
{
	char * unit_name = strtok(line, "_");
	printf("%s\n", unit_name);
	for (int i = 0; unit_name[i] != '\0'; i++)
	{
		unit_name[i] = tolower(unit_name[i]);
	}
	op_code_t opcode = HALT;
	if (strcmp(unit_name, "add") == 0)
	{
		opcode = ADD;
	}
	else if (strcmp(unit_name, "sub") == 0)
	{
		opcode = SUB;
	}
	else if (strcmp(unit_name, "mul") == 0)
	{
		opcode = MULT;
	}
	else if (strcmp(unit_name, "div") == 0)
	{
		opcode = DIV;
	}
	else if (strcmp(unit_name, "ld") == 0)
	{
		opcode = LD;
	}
	else if (strcmp(unit_name, "st") == 0)
	{
		opcode = ST;
	}
	else
	{
		printf("ERROR in get_unit_type_from_cfg_line\n");
		
	}
	return opcode;
}
int init_arch_spec(const char * cfg_path)
{
	FILE * cfg_file = fopen(cfg_path, "r");
	if (cfg_file)
	{
		char cfg_name[MAX_LINE_LEN];
		int line_num = 0;
		int cfg_value = 0;
		while (fscanf(cfg_file, "%s = %d\n", cfg_name, &cfg_value) != EOF)
		{
			printf("%s %d\n", cfg_name, cfg_value);
			op_code_t opcode = get_unit_type_from_cfg_line(cfg_name);
			if (line_num < UNIT_TYPE_NUM)
			{ // we are reading the FUs amounts
				nr_units_array[opcode] = cfg_value;
			}
			else if (line_num >= UNIT_TYPE_NUM && line_num < 2 * UNIT_TYPE_NUM)
			{ // we are reading the FUs delays
				delays_array[opcode] = cfg_value;
			}
			if (line_num == 2 * UNIT_TYPE_NUM - 1)
			{ // the next line is the trace_unit,
			  // so we read it and exit
				
				char trace_unit_val[MAX_LINE_LEN];
				char trace_unit_name[MAX_LINE_LEN];
				int trace_unit_num;
				fscanf(cfg_file, "%s = %[^0-9]%d\n", cfg_name, trace_unit_name, &trace_unit_num);
				
				//char * trace_unit_name = strtok(trace_unit_val, "0123456789");
				op_code_t unit_type = get_unit_type_from_cfg_line(trace_unit_name);
				trace_unit_cfg.unit_num = trace_unit_num;
				trace_unit_cfg.unit_type = unit_type;
				break;
			}
			line_num++;
		}
	}
	return 1;
}

int init_fu_arr(functional_unit ** fu_arr, int num_elements)
{
	for (int i = 0; i < num_elements; i++)
	{
		fu_arr[i] = (functional_unit *)malloc(sizeof(functional_unit));		
	}
	return 1;
}

bool is_trace(functional_unit * fu)
{
	return (fu->unit_type == trace_unit_cfg.unit_type && fu->unit_index == trace_unit_cfg.unit_num);
}
int init_fu(functional_unit * fu, op_code_t unit_type, int unit_index)
{
	fu->is_busy = false;
	fu->unit_type = unit_type;
	fu->unit_index = unit_index;
	fu->time_left = delays_array[unit_type];
	fu->is_waiting_for_writeback = false;
	fu->instruction_num = 0;
	fu->Fi = 0;
	fu->Fj = 0;
	fu->Fk = 0;
	fu->Qj = 0;
	fu->Qk = 0;
	fu->Rj = false;
	fu->Rk = false;
	fu->is_trace = is_trace(fu);
	return 1;
}

int init_functional_units(functional_unit *** fu_arr)
{
	int total_num_fus = 0;
	for (int i = 0; i < UNIT_TYPE_NUM; i++)
	{
		total_num_fus += nr_units_array[i];
	}
	*fu_arr = (functional_unit **)malloc(total_num_fus * sizeof(functional_unit *));
	//fu_array_next = (functional_unit **)malloc(total_num_fus * sizeof(functional_unit *));
	init_fu_arr(*fu_arr, total_num_fus);
	//init_fu_arr(fu_array_next, total_num_fus);
	int units_init_cnt = 0;
	for (int unit_type = 0; unit_type < UNIT_TYPE_NUM; unit_type++)
	{
		int num_fus_of_type = nr_units_array[unit_type];
		for (int unit_num = 0; unit_num < num_fus_of_type; unit_num++)
		{
			//init_fu(fu_array_curr[units_init_cnt], (op_code_t)unit_type, unit_num);
			init_fu((*fu_arr)[units_init_cnt], (op_code_t)unit_type, unit_num);
			units_init_cnt++;
		}

	}
	return 1;
}

int init_instruction_queue(instruction_queue_t * inst_q)
{
	inst_q->free_spots = INST_Q_LEN;
	inst_q->read_ptr = 0;
	inst_q->write_ptr = 0;
	return 1;
}

int init_func(const char * cfg_path, const char * memin_path, const char * memout_path, const char * regout_path, const char * traceinst_path, const char * traceunit_path)
{
	init_registers(reg_file_curr);
	init_registers(reg_filey_next);
	init_memory(memin_path);
	init_arch_spec(cfg_path);
	init_functional_units(&fu_array_curr);
	init_functional_units(&fu_array_next);
	init_instruction_queue(&inst_queue_curr);
	init_instruction_queue(&inst_queue_next);
	return 1;
}