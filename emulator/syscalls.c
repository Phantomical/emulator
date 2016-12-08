#include "emulator.h"

#include <stdio.h>
#include <string.h>

void syscall_1_write_serial(State* st)
{
	uint64_t num_bytes = st->registers[0].u;
	uint64_t location = st->registers[1].u;

	if (num_bytes + location >= st->mem_size)
	{
		interrupt(st, PageFault);
		return;
	}

	st->registers[0].u = fwrite(&st->memory[location], 1, (size_t)num_bytes, stdout);	
}
void syscall_2_read_serial(State* st)
{
	uint64_t num_bytes = st->registers[0].u;
	uint64_t location = st->registers[1].u;

	if (num_bytes + location >= st->mem_size)
	{
		interrupt(st, PageFault);
		return;
	}

	st->registers[0].u = fread(&st->memory[location], 1, (size_t)num_bytes, stdin);
}

syscall_func system_syscalls[256];

const syscall_func* syscalls()
{
	memset(system_syscalls, 0, sizeof(system_syscalls));

	system_syscalls[1] = syscall_1_write_serial;
	system_syscalls[2] = syscall_2_read_serial;

	return system_syscalls;
}
