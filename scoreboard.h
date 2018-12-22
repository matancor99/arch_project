#pragma once
#ifndef SCOREBOARD_H_
#define SCOREBOARD_H_
//#include "scoreboard.cpp"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "structures.h"



int init_registers(register_struct_t * reg_array);
int init_memory(const char * memin_path);
int init_arch_spec(const char * cfg_path);
int init_functional_units(functional_unit ** fu_arr);
int init_fu_arr(functional_unit ** fu_arr, int num_elements);
int init_fu(functional_unit * fu, op_code_t unit_type, int unit_index);
bool is_trace(functional_unit * fu);
int init_instruction_queue(instruction_queue_t * inst_q);
int init_func(const char * cfg_path, const char * memin_path, const char * memout_path, const char * regout_path, const char * traceinst_path, const char * traceunit_path);
inst_struct_t decode_inst(unsigned int hex_inst, int pc);
int fetch();
int issue();
bool read_operands_for_fu(int fu_num);
int read_operands();
float exec_op(float v1, float v2, int imm, op_code_t opcode);
int execute();
int write_back();
int sample_state();
void print_regout(bool is_dbg);
void print_memout(bool is_dbg);
void traceunit();
const char * opcode_num_to_string(int opcode_num);
void init_files(const char * memout_path, const char * regout_path, const char * traceinst_path, const char * traceunit_path); int update_waiting_fus(int dest_reg);
void traceinst(int pc, trace_inst_phase_t phase);
void set_traceinst_fu(int pc, op_code_t fu_type, int fu_idx);
void init_traceinst();
void print_traceinst(bool is_dbg);
bool is_stop_running();
void strip_spaces(char * line);
void parse_cfg_line(const char * cfg_name, char * cfg_val_str);
op_code_t get_unit_type_num_from_unit_name(char * unit_name);
#endif // SCOREBOARD_H_