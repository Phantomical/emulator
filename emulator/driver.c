
#include "emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_OFFSET 2048
#define RIP 1

int main(int argc, char** argv)
{
	State state;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: emulator <boot-program-file> <memory-size>\n");
		return 1;
	}

	FILE* f = fopen(argv[1], "rb");

	if (!f)
	{
		fprintf(stderr, "Unable to find file %s.\n", argv[1]);
		return 1;
	}

	state.mem_size = strtoull(argv[2], NULL, 0);
	state.halt = false;
	state.memory = malloc(state.mem_size);
	
	fread(state.memory + START_OFFSET, 128, 1, f);
	fclose(f);
	
	memset(state.memory, 0, 256 * 8);
	memset(state.registers, 0, sizeof(state.registers));
	memset(state.stat_regs, 0, sizeof(state.stat_regs));
	memcpy(state.syscalls, syscalls(), sizeof(state.syscalls));

	state.stat_regs[RIP] = START_OFFSET;

	while (!state.halt)
	{
		execute(&state);
	}
}
