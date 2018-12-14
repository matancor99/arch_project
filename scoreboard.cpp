#include "scoreboard.h"
//output file names
//char * memout_file_name = "C:\\Users\\kfir\\Documents\\ee_masters\\winter19\\comp_archs\\project\\proj_output\\memout.txt";
//char * regout_file_name = "C:\\Users\\kfir\\Documents\\ee_masters\\winter19\\comp_archs\\project\\proj_output\\regout.txt";
char * memout_file_name = "memout.txt";
char * regout_file_name = "regout.txt";

int memory[MEM_LEN] = { 0 };
int used_mem_len; // number of rows actually used in memory.
				  // will make printing the memout easier by only printing used lines
int program_counter = 0;
int clock = 0; //system cycle count
bool is_halt = false; // when we read halt command we should no longer fetch
// Globals initilazation
int nr_units_array[UNIT_TYPE_NUM];
int delays_array[UNIT_TYPE_NUM];

trace_unit_cfg_t trace_unit_cfg;

int num_fus;
// current_cycle
functional_unit_t ** fu_array_curr;
register_struct_t reg_file_curr[REG_NUM];
instruction_queue_t inst_queue_curr;

// next_cycle
functional_unit_t ** fu_array_next;
register_struct_t reg_file_next[REG_NUM];
instruction_queue_t inst_queue_next;

int init_registers(register_struct_t * reg_array)
{
	for (int i = 0; i < REG_NUM; i++)
	{
		reg_array[i].value = float(i);
		reg_array[i].is_ready = true;
		reg_array[i].fu = NULL;
	}
	return 1;
}

int init_memory(const char * memin_path)
{
	int row_num = 0;
	FILE * mem_file = fopen(memin_path, "r");
	if (mem_file)
	{
		while (fscanf(mem_file, "%x", &memory[row_num]) == 1)
		{
			row_num++;
		}
	}
	used_mem_len = row_num;
	return 1;
}

op_code_t get_unit_type_from_cfg_line(char * line)
{
	char * unit_name = strtok(line, "_");
	//printf("%s\n", unit_name);
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
			//printf("%s %d\n", cfg_name, cfg_value);
			op_code_t opcode = get_unit_type_from_cfg_line(cfg_name);
			if (line_num < UNIT_TYPE_NUM)
			{ // we are reading the FUs amounts
				nr_units_array[opcode] = cfg_value;
			}
			else if (line_num >= UNIT_TYPE_NUM && line_num < 2 * UNIT_TYPE_NUM)
			{ // we are reading the FUs delays
				delays_array[opcode] = cfg_value - 2; // TODO - figure out execution time
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
	fu->is_execute = false;
	fu->is_writeback = false;
	fu->unit_type = unit_type;
	fu->unit_index = unit_index;
	fu->time_left = delays_array[unit_type];
	fu->instruction_num = 0;
	fu->Fi = 0;
	fu->Fj = 0;
	fu->Fk = 0;
	fu->Qj = NULL;
	fu->Qk = NULL;
	fu->Rj = false;
	fu->Rk = false;
	fu->immediate = 0;
	fu->wb_val = 0.0;
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
	num_fus = total_num_fus;
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
	init_registers(reg_file_next);
	init_memory(memin_path);
	init_arch_spec(cfg_path);
	init_functional_units(&fu_array_curr);
	init_functional_units(&fu_array_next);
	init_instruction_queue(&inst_queue_curr);
	init_instruction_queue(&inst_queue_next);
	return 1;
}

inst_struct_t decode_inst(unsigned int hex_inst, int pc)
{
	inst_struct_t decoded_inst;
	decoded_inst.immidiate	= hex_inst			& 0xFFF;
	decoded_inst.src_reg_1	= (hex_inst >> 12)	& 0xF;
	decoded_inst.src_reg_2	= (hex_inst >> 16)	& 0xF;
	decoded_inst.dest_reg	= (hex_inst >> 20)	& 0xF;
	decoded_inst.op_code	= op_code_t((hex_inst >> 24) & 0xF);
	decoded_inst.pc = pc;
	decoded_inst.instruction = hex_inst;
	return decoded_inst;
}

int fetch()
{
	if (queue_is_free(&inst_queue_curr))
	{ // there's a free spot in the queue so we can fetch
		inst_struct_t inst = decode_inst(memory[program_counter], program_counter);
		if (inst.op_code == HALT)
		{
			is_halt = true;
			return 0;
		}
		queue_push(&inst_queue_next, &inst);
		program_counter++;
		//queue_print(&inst_queue_next);
	}
	else
	{
		return 0;
	}
}



void update_fu(int fu_num, inst_struct_t * curr_inst)
{
	if (fu_array_curr[fu_num]->unit_type == ST)
	{ // STORE
		if (reg_file_curr[fu_array_curr[fu_num]->Fj].is_ready)
		{
			fu_array_next[fu_num]->Rj = true;
		}
		else
		{
			fu_array_next[fu_num]->Rj = false;
			fu_array_next[fu_num]->Qj = reg_file_curr[fu_array_curr[fu_num]->Fj].fu;
		}
	}
	else if (fu_array_curr[fu_num]->unit_type == LD)
	{ // LOAD
		reg_file_next[curr_inst->dest_reg].is_ready = false;
		reg_file_next[curr_inst->dest_reg].fu = fu_array_curr[fu_num];
	}
	else
	{ // ARITHMETIC
		// checking if Fj is ready or not
		if (reg_file_curr[fu_array_curr[fu_num]->Fj].is_ready)
		{
			fu_array_next[fu_num]->Rj = true;
		}
		else
		{
			fu_array_next[fu_num]->Rj = false;
			fu_array_next[fu_num]->Qj = reg_file_curr[fu_array_curr[fu_num]->Fj].fu;
		}
		// checking if Fk is ready or not
		if (reg_file_curr[fu_array_curr[fu_num]->Fk].is_ready)
		{
			fu_array_next[fu_num]->Rk = true;
		}
		else
		{
			fu_array_next[fu_num]->Rk = false;
			fu_array_next[fu_num]->Qk = reg_file_curr[fu_array_curr[fu_num]->Fk].fu;
		}
		// if both source registers are ready - we can start execution
		reg_file_next[curr_inst->dest_reg].is_ready = false;
		reg_file_next[curr_inst->dest_reg].fu = fu_array_curr[fu_num];
	}
	return;
}

int issue()
{
	if (queue_is_empty(&inst_queue_curr))
	{ // no instruction left to issue
		return 0;
	}
	inst_struct_t * curr_inst = queue_read(&inst_queue_curr, PEEK);
	for (int i = 0; i < num_fus; i++)
	{
		if (fu_array_curr[i]->unit_type == curr_inst->op_code &&
			!fu_array_curr[i]->is_busy)
		{ // we can issue the instruction
			queue_read(&inst_queue_next, POP);
			fu_array_next[i]->is_busy = true;
			fu_array_next[i]->instruction_num = curr_inst->pc;
			fu_array_next[i]->Fi = curr_inst->dest_reg;
			fu_array_next[i]->Fj = curr_inst->src_reg_1;
			fu_array_next[i]->Fk = curr_inst->src_reg_2;
			fu_array_next[i]->immediate = curr_inst->immidiate;
			fu_array_next[i]->instruction = curr_inst->instruction;
			fu_array_next[i]->cycle_issued = clock;
			update_fu(i, curr_inst);
			return 1;
		}
	}
	return 0;
}

bool read_operands_for_fu(int fu_num)
{
	if (fu_array_curr[fu_num]->unit_type == ST)
	{ // STORE 
		return reg_file_curr[fu_array_curr[fu_num]->Fj].is_ready || reg_file_curr[fu_array_curr[fu_num]->Fj].fu == fu_array_curr[fu_num];
	}
	else if (fu_array_curr[fu_num]->unit_type == LD)
	{ // LOAD
		return true;
	}
	else
	{ // ARITHMETIC
		// checking if Fj and Fk is ready or not and that the FU is not starving itself
		return (reg_file_curr[fu_array_curr[fu_num]->Fj].is_ready || reg_file_curr[fu_array_curr[fu_num]->Fj].fu == fu_array_curr[fu_num]) && 
			   (reg_file_curr[fu_array_curr[fu_num]->Fk].is_ready || reg_file_curr[fu_array_curr[fu_num]->Fk].fu == fu_array_curr[fu_num]);
	}	
}

int read_operands()
{
	for (int i = 0; i < num_fus; i++)
	{
		if (fu_array_curr[i]->is_busy && !fu_array_curr[i]->is_execute && !fu_array_curr[i]->is_writeback)
		{ // this means our FU is in ReadOperand stage
			if (read_operands_for_fu(i))
			{
				fu_array_next[i]->is_execute = true;		
				fu_array_next[i]->cycle_read_operands = clock;
			}
		}
	}
	return 1;
}

float exec_op(float v1, float v2, int imm, op_code_t opcode)
{
	printf("exec_op: v1 = %f, v2 = %f, imm = %d, opcode = %d\n", v1, v2, imm, opcode);
	switch (opcode)
	{
	case ADD: return v1 + v2;
	case SUB: return v1 - v2;
	case MULT: return v1 * v2;
	case DIV: return v1 / v2;
	case LD: float f; f = *((float*)&memory[imm]);
		printf("f = %f\n", f);
			 return f;
	case ST: memory[imm] = *((int*)&v1);
			 return 0.0;
	default: printf("ERROR in exec_op - invalid command!\n");
			 return 1.0;
	}
}


int execute()
{
	for (int i = 0; i < num_fus; i++)
	{
		if (fu_array_curr[i]->is_execute)
		{
			functional_unit_t * fu_curr = fu_array_curr[i];
			if (fu_array_curr[i]->time_left == 0)
			{ // execution ended!
				//printf("execute: type: %d, val1 = %f, val2 = %f, imm = %d", reg_file_curr[fu_curr->unit_type], reg_file_curr[fu_curr->Fj].value, reg_file_curr[fu_curr->Fk].value, fu_curr->immediate);
				fu_array_next[i]->wb_val = exec_op(reg_file_curr[fu_curr->Fj].value, reg_file_curr[fu_curr->Fk].value, fu_curr->immediate, fu_curr->unit_type);
				fu_array_next[i]->is_execute = false;
				fu_array_next[i]->is_writeback = true;
				fu_array_next[i]->cycle_execute_end = clock;

			}
			else
			{
				fu_array_next[i]->time_left--;
			}
		}
	}
	return 1;
}

int write_back()
{
	for (int i = 0; i < num_fus; i++)
	{
		if (fu_array_curr[i]->is_writeback)
		{
			int dest_reg = fu_array_curr[i]->Fi;
			
			for (int j = 0; j < num_fus; j++)
			{
				// handle WAR hazards:
				// looking for FUs with intruction prior to this one that have
				// the current instruction's dest_reg as it's source, and are still
				// waiting to read operands
				if (fu_array_curr[j]->is_busy && !fu_array_curr[j]->is_execute && !fu_array_curr[j]->is_writeback
					&& ( dest_reg == fu_array_curr[j]->Fj || dest_reg == fu_array_curr[j]->Fk)
					&& fu_array_curr[j]->instruction_num < fu_array_curr[i]->instruction_num)
				{ // WAR!
					break;
				}
				// handle WAW hazards:
				// looking for FUs with instruction prior to this one that have
				// the current instruction's dest_reg as it's dest, and are yet to 
				// writeback their values
				else if (fu_array_curr[j]->is_busy
						&& dest_reg == fu_array_curr[j]->Fi
						&& fu_array_curr[j]->instruction_num < fu_array_curr[i]->instruction_num)
				{ // WAW!
					break;
				}
				else
				{ // Writeback
					reg_file_next[dest_reg].is_ready = true;
					reg_file_next[dest_reg].value = fu_array_curr[i]->wb_val;
					fu_array_curr[i]->cycle_write_result = clock;
					fu_print(fu_array_curr[i]);
					init_fu(fu_array_next[i], fu_array_next[i]->unit_type, fu_array_next[i]->unit_index);
					break;
				}

			}
		}
	}
	return 1;
}

int sample_state()
{
	for (int i = 0; i < num_fus; i++)
	{
		memcpy(fu_array_curr[i], fu_array_next[i], sizeof(*fu_array_curr[i]));
	}
	memcpy(reg_file_curr, reg_file_next, sizeof(reg_file_curr));
	memcpy(&inst_queue_curr, &inst_queue_next, sizeof(inst_queue_curr));
	clock++;
	return 1;
}

void print_regout(bool is_dbg)
{
	FILE * regout;
	if (!is_dbg)
	{
		regout = fopen(regout_file_name, "wb");
		if (!regout)
		{
			printf("print_regout file open error!\n");
			return;
		}
	}	
	for (int i = 0; i < REG_NUM; i++)
	{
		char reg_line[40];
		sprintf(reg_line, "%f\n", reg_file_curr[i].value);
		if (is_dbg)
		{
			printf("%f\n", reg_file_curr[i].value);
		}
		else
		{			
			fputs(reg_line, regout);
		}

	}
	if (!is_dbg)
	{
		fclose(regout);
	}
}

void print_memout(bool is_dbg)
{
	FILE * memout;
	if (!is_dbg)
	{
		memout = fopen(memout_file_name, "wb");
		if (!memout)
		{
			printf("print_memout file open error!\n");
			return;
		}
	}
	for (int i = 0; i < used_mem_len; i++)
	{
		char mem_line[12];
		sprintf(mem_line, "%08x\n", memory[i]);
		if (is_dbg)
		{
			printf(mem_line);
		}
		else
		{
			fputs(mem_line, memout);
		}
	}
	if (!is_dbg)
	{
		fclose(memout);

	}
}