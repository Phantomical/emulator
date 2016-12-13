
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

typedef union
{
	// Header common to all instruction types
	struct Common
	{
		uint32_t opcode : 7;
		uint32_t inst_t : 2;
	} cmn;
	struct Type1
	{
		uint32_t opcode : 7;
		uint32_t inst_t : 2;
		uint32_t config : 2;
		uint32_t dst : 5;
		uint32_t src : 5;
		uint32_t extra : 11;
	} t1;
	struct Type2
	{
		uint32_t opcode : 7;
		uint32_t inst_t : 2;
		uint32_t config : 2;
		uint32_t dst : 5;
		uint32_t immediate : 16;
	} t2;
	struct Type3
	{
		uint32_t opcode : 7;
		uint32_t inst_t : 2;
		uint32_t config : 4;
		uint32_t immediate : 19;
	} t3;
	uint32_t total;
} Instruction;

uint32_t byteswap32(uint32_t num)
{
	return ((num>>24)&0xff)       // move byte 3 to byte 0
		| ((num<<8)&0xff0000)     // move byte 1 to byte 2
		| ((num>>8)&0xff00)       // move byte 2 to byte 1
		| ((num<<24)&0xff000000); // byte 0 to byte 3
}

int main(int argc, char** argv)
{
	uint32_t args[6];
	Instruction inst;

	if (argc < 5)
	{
		fprintf(stderr, "Not enough arguments\n");
		return 1;
	}

	for (int i = 1; i < argc && i < 7; ++i)
	{
		args[i - 1] = (uint32_t)strtoul(argv[i], NULL, 16);
	}

	switch (args[1])
	{
	case 0:
		inst.t1.opcode = args[0];
		inst.t1.inst_t = args[1];
		inst.t1.config = args[2];
		inst.t1.dst = args[3];
		inst.t1.src = args[4];
		inst.t1.extra = args[5];
		break;
	case 1:
		inst.t2.opcode = args[0];
		inst.t2.inst_t = args[1];
		inst.t2.config = args[2];
		inst.t2.dst = args[3];
		inst.t2.immediate = args[4];
		break;
	case 2:
		inst.t3.opcode = args[0];
		inst.t3.inst_t = args[1];
		inst.t3.config = args[2];
		inst.t3.immediate = args[3];
		break;
	default:
		fprintf(stderr, "No instruction type %d.\n", args[1]);
		return 1;
	}

	uint32_t value = inst.total;
	printf("%08" PRIu32 "X\n", byteswap32(value));
}

