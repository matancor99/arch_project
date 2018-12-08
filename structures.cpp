#include "structures.h"

bool queue_is_free(instruction_queue_t * q)
{
	if (q->free_spots == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int queue_push(instruction_queue_t * q, inst_struct_t * inst)
{
	if (queue_is_free(q))
	{
		q->inst_array[q->write_ptr] = *inst;
		q->write_ptr = (q->write_ptr + 1) % 16;
		q->free_spots--;
		return 1;
	}
	else
	{
		return 0;
	}
}

inst_struct_t * queue_read(instruction_queue_t * q, queue_command_t q_cmd)
{
	if (q->free_spots == INST_Q_LEN)
	{
		return 0;
	}
	else
	{
		inst_struct_t * ret_inst = (inst_struct_t *)malloc(sizeof(inst_struct_t));
		ret_inst = &q->inst_array[q->read_ptr];
		if (q_cmd == POP)
		{
			q->read_ptr = (q->read_ptr + 1) % 16;
			q->free_spots++;
		}
		return ret_inst;
	}
}

void queue_print(instruction_queue_t * q)
{
	printf("printing q\n");
	printf("read_ptr = %d\twrite_ptr = %d\t free_spots =%d\n", q->read_ptr, q->write_ptr, q->free_spots);
	for (int i = q->read_ptr; i < q->write_ptr; i++)
	{
		inst_print(&q->inst_array[i]);
	}
}

void inst_print(inst_struct_t * inst)
{
	printf("op_code = %d\tdest_reg = %d\tsrc_reg_1 = %d\tsrc_reg_2 = %d\timm = %d\n", inst->op_code, inst->dest_reg, inst->src_reg_1, inst->src_reg_2, inst->immidiate);
}