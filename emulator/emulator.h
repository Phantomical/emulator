#pragma once

#include <stdbool.h>

#define uint32_t unsigned int
#define uint64_t unsigned long long
#define uint8_t unsigned char
#define int64_t long long
#define int32_t int
#define uint16_t unsigned short
#define int16_t short
#define int8_t char

enum OpCodes
{
	NOP = 0x00,
	INT = 0x01,
	SYSCALL = 0x02,
	ADD = 0x03,
	SUB = 0x04,
	MUL = 0x05,
	DIV = 0x06,
	MOV = 0x07,
	LD = 0x08,
	ST = 0x09,
	MOD = 0x0A,
	AND = 0x0B,
	OR = 0x0C,
	XOR = 0x0D,
	NOT = 0x0E,
	NEG = 0x0F,
	SHL = 0x10,
	SHR = 0x11,
	HALT = 0x12,
	JMP = 0x13,
	Jx = 0x14,
	CMP = 0x15,
	FCONV = 0x16,
	NUM_OPCODES
};

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
} Instruction;
typedef union
{
	uint64_t u;
	int64_t i;
	double d;
	struct { uint32_t p; float v; } f;
} Register;

typedef struct vState State;

typedef void(*syscall_func)(State*);

struct vState
{
	syscall_func syscalls[256];
	Register registers[32];
	uint64_t stat_regs[2];
	uint8_t* memory;
	bool halt;
	uint64_t mem_size;
};

enum Flags
{
	ZF = 0x1,
	OF = 0x2,
	CF = 0x4,
	SF = 0x8
};
enum Interrupts
{
	Default = 0x0,
	InvalidInstruction = 0x01,
	Breakpoint = 0x02,
	DivideBy0 = 0x03,
	PageFault = 0x04,
	DebugInterrupt = 0x05,
	GeneralProtectionFault = 0x06
};
enum JmpType
{
	// Base Jump Instructions
	JZ = 0x0,
	JNZ = 0x1,
	JO = 0x2,
	JNO = 0x3,
	JC = 0x4,
	JNC = 0x5,
	JS = 0x6,
	JNS = 0x7,

	// Compound Jump Instructions
	JA = 0x8,
	JG = 0x9,

	JBE = 0xA,
	JLE = 0xB,

	// Aliases
	JAE = JNC,
	JGE = JNS,
	JE = JZ,
	JNE = JNZ,
	JB = JC,
	JL = JS,

	JNA = JBE,
	JNAE = JB,
	JNG = JLE,
	JNGE = JL,
	JNB = JAE,
	JNBE = JA,
	JNL = JGE,
	JNLE = JG
};

void halt(State* st);
void execute(State* st);
void interrupt(State* st, uint8_t interrupt);

const syscall_func* syscalls();
