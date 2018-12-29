#pragma once
#ifndef SCOREBOARD_H_
#define SCOREBOARD_H_
//#include "scoreboard.cpp"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "structures.h"


// initialize reg_array so that Fi=i 
int init_registers(register_struct_t * reg_array);
// initialize memory according to memin_path file
int init_memory(const char * memin_path);
// initialize delays_array, nr_units_array and trace_inst_cfg according to cfg_path file
int init_arch_spec(const char * cfg_path);
// initialize fu_arr and all its internal fields
int init_functional_units(functional_unit ** fu_arr);
// initialize an array of FUs with num_elements elements
int init_fu_arr(functional_unit ** fu_arr, int num_elements);
// initialize fu of type unit_type and index unit_index
int init_fu(functional_unit * fu, op_code_t unit_type, int unit_index);
// return true if fu == trace_unit_cfg or false otherwise
bool is_trace(functional_unit * fu);
// initialize inst_q and all its fields
int init_instruction_queue(instruction_queue_t * inst_q);
// intialize the entire HW in the system and all the globals used
int init_func(const char * cfg_path, const char * memin_path, const char * memout_path, const char * regout_path, const char * traceinst_path, const char * traceunit_path);
// decode a hex value of instruction number=pc and value=hex_inst to all the different fields it's constructed of
inst_struct_t decode_inst(unsigned int hex_inst, int pc);
// fetch next instruction to the instruction queue
int fetch();
// issue the next instruction from the instruction queue to the relevant and free FU
int issue();
// returns true if FU[fu_num] can read its operands, false otherwise
bool read_operands_for_fu(int fu_num);
// read operands for all ready FUs
int read_operands();
// execute an operation of type opcode using v1,v2,imm
float exec_op(float v1, float v2, int imm, op_code_t opcode);
// advance execution in all ready FUs
int execute();
// writeback instruction in all ready FUs
int write_back();
// sample state from all _next globals to _curr
int sample_state();
// print regout information to file if is_dbg is true, else print to stdout
void print_regout(bool is_dbg);
// print memout information to file if is_dbg is true, else print to stdout
void print_memout(bool is_dbg);
// print next row of traceunit
void traceunit();
// convert opcode_num to its mnemonic
const char * opcode_num_to_string(int opcode_num);
// init all global file paths
void init_files(const char * memout_path, const char * regout_path, const char * traceinst_path, const char * traceunit_path); 
// update Rj and Rk of all FUs waiting for dest_reg
int update_waiting_fus(int dest_reg);
// update tracing information for command number = pc and phase = phase with the current cycle
void traceinst(int pc, trace_inst_phase_t phase);
// update traceinst_arr with of the FU that is executing command pc
void set_traceinst_fu(int pc, op_code_t fu_type, int fu_idx);
// init the traceinst_arr global
void init_traceinst();
// print traceinst information to file if is_dbg is true, else print to stdout
void print_traceinst(bool is_dbg);
// returns true if our simulator should end, otherwise false
bool is_stop_running();
// remove all spaces from line
void strip_spaces(char * line);
// set the globals relevant according to cfg_name with value from cfg_val_str
void parse_cfg_line(const char * cfg_name, char * cfg_val_str);
// converts unit_name to an op_code_t
op_code_t get_unit_type_num_from_unit_name(char * unit_name);
// free all allocated memory
void cleanup_function();
#endif // SCOREBOARD_H_