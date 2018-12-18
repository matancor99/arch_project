#include "scoreboard.h"
//output file names
//char * memout_file_name = "C:\\Users\\kfir\\Documents\\ee_masters\\winter19\\comp_archs\\project\\proj_output\\memout.txt";
//char * regout_file_name = "C:\\Users\\kfir\\Documents\\ee_masters\\winter19\\comp_archs\\project\\proj_output\\regout.txt";
const char * memout_file_name = "memout.txt";
const char * regout_file_name = "regout.txt";
const char * traceunit_file_name = "traceunit.txt";
const char * traceinst_file_name = "traceinst.txt";


traceinst_t * traceinst_arr;
int num_insts;

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
			op_code_t opcode = get_unit_type_from_cfg_line(cfg_name);
			if (line_num < UNIT_TYPE_NUM)
			{ // we are reading the FUs amounts
				nr_units_array[opcode] = cfg_value;
			}
			else if (line_num >= UNIT_TYPE_NUM && line_num < 2 * UNIT_TYPE_NUM)
			{ // we are reading the FUs delays
				delays_array[opcode] = cfg_value - 1; // TODO - figure out execution time
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

void init_files()
{
	FILE * file_traceunit = fopen(traceunit_file_name, "wb");
	if (file_traceunit)
	{
		fclose(file_traceunit);
	}
}

void init_traceinst()
{
	// find number of commands in program by reading memory until HALT
	bool is_reached_halt = false;
	int num_cmds = 0;
	while (!is_reached_halt)
	{
		if (decode_inst(memory[num_cmds], num_cmds).op_code != HALT)
		{
			num_cmds++;
		}
		else
		{
			is_reached_halt = true;
		}
	}
	num_insts = num_cmds;
	traceinst_arr = (traceinst_t *)malloc(sizeof(traceinst_t) * num_cmds);
	for (int i = 0; i < num_cmds; i++)
	{
		traceinst_arr[i].issued = 0;
		traceinst_arr[i].read_operands = 0;
		traceinst_arr[i].exec = 0;
		traceinst_arr[i].wb = 0;
		traceinst_arr[i].pc = i;
		traceinst_arr[i].command_hex = memory[i];
	}
}

int init_func(const char * cfg_path, const char * memin_path, const char * memout_path, const char * regout_path, const char * traceinst_path, const char * traceunit_path)
{
	init_files();
	init_registers(reg_file_curr);
	init_registers(reg_file_next);
	init_memory(memin_path);
	init_traceinst();
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
	decoded_inst.src_reg_2	= (hex_inst >> 12)	& 0xF;
	decoded_inst.src_reg_1	= (hex_inst >> 16)	& 0xF;
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
	}
	else
	{
		return 0;
	}
}



void update_fu(int fu_num, inst_struct_t * curr_inst)
{
	if (curr_inst->op_code == ST)
	{ // STORE
		if (reg_file_curr[curr_inst->src_reg_2].is_ready)
		{
			fu_array_next[fu_num]->Rj = true;
		}
		else
		{
			fu_array_next[fu_num]->Rj = false;
			fu_array_next[fu_num]->Qj = reg_file_curr[curr_inst->src_reg_2].fu;
		}
	}
	else if (curr_inst->op_code == LD)
	{ // LOAD
		reg_file_next[curr_inst->dest_reg].is_ready = false;
		reg_file_next[curr_inst->dest_reg].fu = fu_array_curr[fu_num];
	}
	else
	{ // ARITHMETIC
		// checking if Fj is ready or not
		if (reg_file_curr[curr_inst->src_reg_1].is_ready)
		{
			fu_array_next[fu_num]->Rj = true;
		}
		else
		{
			fu_array_next[fu_num]->Rj = false;
			fu_array_next[fu_num]->Qj = reg_file_curr[curr_inst->src_reg_1].fu;
		}
		// checking if Fk is ready or not
		if (reg_file_curr[curr_inst->src_reg_2].is_ready)
		{
			fu_array_next[fu_num]->Rk = true;
		}
		else
		{
			fu_array_next[fu_num]->Rk = false;
			fu_array_next[fu_num]->Qk = reg_file_curr[curr_inst->src_reg_2].fu;
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
			update_fu(i, curr_inst);
			traceinst(curr_inst->pc, ISSUE);
			return 1;
		}
	}
	return 0;
}

void traceinst(int pc, trace_inst_phase_t phase)
{
	switch (phase)
	{
	case ISSUE: traceinst_arr[pc].issued = clock; break;
	case READ_OPERANDS: traceinst_arr[pc].read_operands = clock; break;
	case EXEC: traceinst_arr[pc].exec = clock; break;
	case WB: traceinst_arr[pc].wb = clock; break;
	default: printf("traceinst ERROR!\n");
	}	
}

bool read_operands_for_fu(int fu_num)
{
	if (fu_array_curr[fu_num]->unit_type == ST)
	{ // STORE 
		bool cond1 = false;
		if (reg_file_curr[fu_array_curr[fu_num]->Fk].is_ready ||
			reg_file_curr[fu_array_curr[fu_num]->Fk].fu == fu_array_curr[fu_num])
		{
			cond1 = true;
		}
		// we look for previous loads from the same address which have not yet finished
		// if we find any - we wait
		bool cond2 = true;
		for (int i = 0; i < num_fus; i++)
		{
			if (fu_array_curr[i]->unit_type == LD &&
				fu_array_curr[i]->is_busy &&
				fu_array_curr[i]->immediate == fu_array_curr[fu_num]->immediate &&
				fu_array_curr[i]->instruction_num < fu_array_curr[fu_num]->instruction_num)
			{
				return false;
			}
		}
		return cond1 && cond2;
	}
	else if (fu_array_curr[fu_num]->unit_type == LD)
	{	// LOAD
		// we look for previous stores to the same address which have not yet finished
		// if we find any - we wait
		for (int i = 0; i < num_fus; i++)
		{
			if (fu_array_curr[i]->unit_type == ST &&
				fu_array_curr[i]->is_busy &&
				fu_array_curr[i]->immediate == fu_array_curr[fu_num]->immediate &&
				fu_array_curr[i]->instruction_num < fu_array_curr[fu_num]->instruction_num)
			{
				return false;
			}
		}
		return true;
	}
	else
	{ // ARITHMETIC
		// checking if Fj and Fk is ready or not and that the FU is not starving itself
		functional_unit_t * Fj_waiting_for;
		functional_unit_t * Fk_waiting_for;
		functional_unit_t * curr_fu = fu_array_curr[fu_num];
		bool is_Fj_ready = reg_file_curr[curr_fu->Fj].is_ready;
		bool is_Fk_ready = reg_file_curr[curr_fu->Fk].is_ready;


		if (is_Fj_ready && is_Fk_ready)
		{ // both src regs are ready - we can read operands!
			return true;
		}
		else if (is_Fj_ready || is_Fk_ready)
		{ // one of the src regs is ready 
			if (reg_file_curr[curr_fu->Fj].fu == curr_fu || reg_file_curr[curr_fu->Fk].fu == curr_fu)
			{ // means one of the src regs is also the dest register - so we can read it
				return true;
			}
		}
		return false;
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
				fu_array_next[i]->Rj = false;
				fu_array_next[i]->Rk = false;
				fu_array_next[i]->Qj = NULL;
				fu_array_next[i]->Qk = NULL;
				fu_array_next[i]->is_execute = true;		
				traceinst(fu_array_curr[i]->instruction_num, READ_OPERANDS);
			}
		}
	}
	return 1;
}

float exec_op(float v1, float v2, int imm, op_code_t opcode)
{
	switch (opcode)
	{
	case ADD: return v1 + v2;
	case SUB: return v1 - v2;
	case MULT: return v1 * v2;
	case DIV: return v1 / v2;
	case LD: float f; f = *((float*)&memory[imm]);
			 return f;
	case ST: memory[imm] = *((int*)&v2);
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
			if (fu_array_curr[i]->time_left == 1 || fu_array_curr[i]->time_left == 0)
			{ // execution ended!
				fu_array_next[i]->wb_val = exec_op(reg_file_curr[fu_curr->Fj].value, reg_file_curr[fu_curr->Fk].value, fu_curr->immediate, fu_curr->unit_type);
				fu_array_next[i]->is_execute = false;
				fu_array_next[i]->is_writeback = true;
				traceinst(fu_curr->instruction_num, EXEC);
				set_traceinst_fu(fu_curr->instruction_num, fu_curr->unit_type, fu_curr->unit_index);
			}
			else
			{
				
				fu_array_next[i]->time_left--;
				//if (fu_array_curr[i]->time_left == delays_array[fu_array_curr[i]->unit_type] &&
				//	fu_array_next[i]->time_left > 0)
				//{ // this is the first cycle of execution, so we decrement time_left again because
				//  // execution "began" when we read the operands
				//	fu_array_next[i]->time_left--;
				//}
			}
		}
	}
	return 1;
}


int update_waiting_fus(int dest_reg)
{
	for (int i = 0; i < num_fus; i++)
	{
		if (fu_array_curr[i]->Fj == dest_reg && !fu_array_curr[i]->Rj)
		{
			fu_array_next[i]->Rj = true;
		}
		if (fu_array_curr[i]->Fk == dest_reg && !fu_array_curr[i]->Rk)
		{
			fu_array_next[i]->Rk = true;
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
					update_waiting_fus(dest_reg);
					traceinst(fu_array_curr[i]->instruction_num, WB);
					//fu_print(fu_array_curr[i]);
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
	FILE * regout = NULL;
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
		if (is_dbg)
		{
			printf("%f\n", reg_file_curr[i].value);
		}
		else
		{
			char reg_val[12];
			sprintf(reg_val, "%f\n", reg_file_curr[i].value);
			fputs(reg_val, regout);
		}

	}
	if (!is_dbg)
	{
		fclose(regout);
	}
}

void print_memout(bool is_dbg)
{
	FILE * memout = NULL;
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


void traceunit()
{
	for (int i = 0; i < num_fus; i++)
	{
		if (fu_array_curr[i]->is_trace && fu_array_curr[i]->is_busy)
		{
			FILE * file_traceunit = fopen(traceunit_file_name, "a");
			if (file_traceunit)
			{
				char traceunit_line[MAX_LINE_LEN];
				char Fi[MAX_LINE_LEN];
				char Fj[MAX_LINE_LEN];
				char Fk[MAX_LINE_LEN];
				char Qj[MAX_LINE_LEN];
				char Qk[MAX_LINE_LEN];
				char Rj[MAX_LINE_LEN];
				char Rk[MAX_LINE_LEN];
				int cycle = clock;
				char unit[MAX_LINE_LEN];
				sprintf(unit, "%s%d", opcode_num_to_string(fu_array_curr[i]->unit_type), fu_array_curr[i]->unit_index);
				switch (fu_array_curr[i]->unit_type)
				{
				case MULT:
				case DIV:
				case SUB:
				case ADD:
					sprintf(Fi, "F%d", fu_array_curr[i]->Fi);
					sprintf(Fj, "F%d", fu_array_curr[i]->Fj);
					sprintf(Fk, "F%d", fu_array_curr[i]->Fk);
					if (fu_array_curr[i]->Rj)
					{
						sprintf(Rj, "Yes");
						sprintf(Qj, "-");
					}
					else
					{
						sprintf(Rj, "No");
						if (fu_array_curr[i]->Qj)
						{
							sprintf(Qj, "%s%d", opcode_num_to_string(fu_array_curr[i]->Qj->unit_type), fu_array_curr[i]->Qj->unit_index);
						}
						else
						{
							sprintf(Qj, "-");
						}
					}
					if (fu_array_curr[i]->Rk)
					{
						sprintf(Rk, "Yes");
						sprintf(Qk, "-");
					}
					else
					{
						sprintf(Rk, "No");
						if (fu_array_curr[i]->Qk)
						{
							sprintf(Qk, "%s%d", opcode_num_to_string(fu_array_curr[i]->Qk->unit_type), fu_array_curr[i]->Qk->unit_index);
						}
						else
						{
							sprintf(Qk, "-");
						}
					}					
					break;
				case LD:
					sprintf(Fi, "F%d", fu_array_next[i]->Fi);
					sprintf(Fj, "-");
					sprintf(Fk, "-");
					sprintf(Rj, "Yes");
					sprintf(Qj, "-");
					sprintf(Rk, "Yes");
					sprintf(Qk, "-");
					break;
				case ST:
					sprintf(Fi, "-");
					sprintf(Fj, "-");
					sprintf(Fk, "F%d", fu_array_next[i]->Fk);
					sprintf(Rj, "Yes");
					sprintf(Qj, "-");
					if (fu_array_curr[i]->Rk)
					{
						sprintf(Rk, "Yes");
						sprintf(Qk, "-");
					}
					else
					{
						sprintf(Rk, "No");
						if (fu_array_curr[i]->Qk)
						{
							sprintf(Qk, "%s%d", opcode_num_to_string(fu_array_curr[i]->Qk->unit_type), fu_array_curr[i]->Qk->unit_index);
						}
						else
						{
							sprintf(Qk, "-");
						}
					}
					
					break;
				default:
					break;
				}
				fprintf(file_traceunit, "%d %s %s %s %s %s %s %s %s\n", cycle, unit, Fi, Fj, Fk, Qj, Qk, Rj, Rk);			
				fclose(file_traceunit);
				return;
			}
		}
	}
}


const char * opcode_num_to_string(int opcode_num)
{
	switch (opcode_num)
	{
	case LD: return "LD";
	case ST: return "ST";
	case ADD: return "ADD";
	case SUB: return "SUB";
	case MULT: return "MULT";
	case DIV: return "DIV";
	case HALT: return "HALT";
	default: return "ERROR";
	}
}

void print_traceinst(bool is_dbg)
{
	FILE * file_traceinst = fopen(traceinst_file_name, "wb");
	if (!file_traceinst)
	{
		return;
	}
	for (int i = 0; i < num_insts; i++)
	{
		fprintf(file_traceinst, "%08x %d %s%d %d %d %d %d\n",
			traceinst_arr[i].command_hex,
			traceinst_arr[i].pc,
			opcode_num_to_string(traceinst_arr[i].fu_type),
			traceinst_arr[i].fu_idx,
			traceinst_arr[i].issued,
			traceinst_arr[i].read_operands,
			traceinst_arr[i].exec,
			traceinst_arr[i].wb);
	}
	fclose(file_traceinst);
}

void set_traceinst_fu(int pc, op_code_t fu_type, int fu_idx)
{
	traceinst_arr[pc].fu_type = fu_type;
	traceinst_arr[pc].fu_idx = fu_idx;
}


bool is_stop_running()
{
	for (int i = 0; i < num_fus; i++)
	{
		if (fu_array_curr[i]->is_busy)
		{// we still have active FUs - keep running
			return false;
		}
	}
	return is_halt; // if all the FUs are idle, and we have nothing left to fetch - stop.
}
