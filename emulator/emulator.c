
#include "emulator.h"

#include <string.h>
#include <assert.h>

#if defined _WIN32
#include <intrin.h>
uint64_t clz(uint64_t x)
{
	return x == 0 ? 64 : __lzcnt64(x);
}

bool __builtin_saddll_overflow(int64_t x, int64_t y, int64_t* z) { return false; }
bool __builtin_ssubll_overflow(int64_t x, int64_t y, int64_t* z) { return false; }
bool __builtin_smulll_overflow(int64_t x, int64_t y, int64_t* z) { return false; }

bool __builtin_uaddll_overflow(uint64_t x, uint64_t y, uint64_t*z) { return false; }
bool __builtin_usubll_overflow(uint64_t x, uint64_t y, uint64_t*z) { return false; }
bool __builtin_umulll_overflow(uint64_t x, uint64_t y, uint64_t*z) { return false; }
#elif defined __GNUC__
uint64_t clz(uint64_t value)
{
	return value = 0 ? 64 : __builtin_clzll(value);
}
#else
const uint64_t tab64[64] = {
	63,  0, 58,  1, 59, 47, 53,  2,
	60, 39, 48, 27, 54, 33, 42,  3,
	61, 51, 37, 40, 49, 18, 28, 20,
	55, 30, 34, 11, 43, 14, 22,  4,
	62, 57, 46, 52, 38, 26, 32, 41,
	50, 36, 17, 19, 29, 10, 13, 21,
	56, 45, 25, 31, 35, 16,  9, 12,
	44, 24, 15,  8, 23,  7,  6,  5 };

uint64_t clz(uint64_t value)
{
	if (value == 0)
		return 64;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value |= value >> 32;
	return 63 - tab64[((uint64_t)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}
#endif

#define CHECKED_SADD(x, y, res) __builtin_saddll_overflow(x, y, res)
#define CHECKED_SSUB(x, y, res) __builtin_ssubll_overflow(x, y, res)
#define CHECKED_SMUL(x, y, res) __builtin_smulll_overflow(x, y, res)

#define CHECKED_UADD(x, y, res) __builtin_uaddll_overflow(x, y, res)
#define CHECKED_USUB(x, y, res) __builtin_usubll_overflow(x, y, res)
#define CHECKED_UMUL(x, y, res) __builtin_umulll_overflow(x, y, res)

#define SEXT_DEF(sz) \
	int64_t sext##sz(uint64_t x) \
	{ \
		if (x & (1 << sz)) \
			return -(1 << (sz - 1)) + (int64_t)(x & ((1 << (sz - 1)) - 1)); \
		return (int64_t)x; \
	} 

SEXT_DEF(16);
SEXT_DEF(19);

void halt(State* st)
{
	st->halt = true;
}
void interrupt(State* st, uint8_t code)
{
	uint64_t interrupt = *(uint64_t*)(st->memory + code * 8);
	if (interrupt == 0)
	{
		interrupt = *(uint64_t*)(st->memory);
		if (interrupt == 0)
			halt(st);
	}

	st->stat_regs[1] = interrupt;
}

#define setFlag(flag, cond) do { if (cond) st->stat_regs[0] |= flag; else st->stat_regs[0] &= ~flag; } while(0)
#define dstreg(type) st->registers[inst.type.dst]
#define srcreg(type) st->registers[inst.type.src]
#define immed(type) inst.type.immediate
#define config(type) inst.type.config

void add(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
			setFlag(CF, CHECKED_UADD(dstreg(t1).u, srcreg(t1).u, &dstreg(t1).u));
			break;
		case 0b01:
			setFlag(OF, CHECKED_SADD(dstreg(t1).i, srcreg(t1).i, &dstreg(t1).i));
			break;
		case 0b10:
			dstreg(t1).f.v = dstreg(t1).f.v + srcreg(t1).f.v;
			dstreg(t1).f.p = 0;
			break;
		case 0b11:
			dstreg(t1).d = dstreg(t1).d + srcreg(t1).d;
			break;
		}
		break;
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
			setFlag(CF, CHECKED_UADD(dstreg(t2).u, immed(t2), &dstreg(t2).u));
			break;
		case 0b01:
			setFlag(OF, CHECKED_SADD(dstreg(t2).i, sext16(immed(t2)), &dstreg(t2).i));
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void sub(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
			setFlag(CF, CHECKED_USUB(dstreg(t1).u, srcreg(t1).u, &dstreg(t1).u));
			break;
		case 0b01:
			setFlag(OF, CHECKED_SSUB(dstreg(t1).i, srcreg(t1).i, &dstreg(t1).i));
			break;
		case 0b10:
			dstreg(t1).f.v = dstreg(t1).f.v - srcreg(t1).f.v;
			dstreg(t1).f.p = 0;
			break;
		case 0b11:
			dstreg(t1).d = dstreg(t1).d - srcreg(t1).d;
			break;
		}
		break;
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
			setFlag(CF, CHECKED_USUB(dstreg(t2).u, immed(t2), &dstreg(t2).u));
			break;
		case 0b01:
			setFlag(OF, CHECKED_SSUB(dstreg(t2).i, sext16(immed(t2)), &dstreg(t2).i));
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void mul(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
			setFlag(CF, CHECKED_UMUL(dstreg(t1).u, srcreg(t1).u, &dstreg(t1).u));
			break;
		case 0b01:
			setFlag(OF, CHECKED_SMUL(dstreg(t1).i, srcreg(t1).i, &dstreg(t1).i));
			break;
		case 0b10:
			dstreg(t1).f.v = dstreg(t1).f.v * srcreg(t1).f.v;
			dstreg(t1).f.p = 0;
			break;
		case 0b11:
			dstreg(t1).d = dstreg(t1).d * srcreg(t1).d;
			break;
		}
		break;
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
			setFlag(CF, CHECKED_UMUL(dstreg(t2).u, immed(t2), &dstreg(t2).u));
			break;
		case 0b01:
			setFlag(OF, CHECKED_SMUL(dstreg(t2).i, sext16(immed(t2)), &dstreg(t2).i));
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void div(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
			if (srcreg(t1).u == 0)
				interrupt(st, DivideBy0);
			else
				dstreg(t1).u /= srcreg(t1).u;
			break;
		case 0b01:
			if (srcreg(t1).i == 0)
				interrupt(st, DivideBy0);
			else
				// NOTE: Will fail to set correct flags when
				//       INT_MIN is divided by -1
				dstreg(t1).i /= srcreg(t1).i;
			break;
		case 0b10:
			dstreg(t1).f.v /= srcreg(t1).f.v;
			dstreg(t1).f.p = 0;
			break;
		case 0b11:
			dstreg(t1).d /= srcreg(t1).d;
			break;
		}
		break;
	case 0b01:
		if (immed(t2) == 0 || config(t2) & 0b10) // t2.config is not 00 or 01
		{
			interrupt(st, InvalidInstruction);
			return;
		}

		if (config(t2) == 0)
			dstreg(t2).u /= immed(t2);
		else
			// NOTE: Will fail to set correct flags when
			//       INT_MIN is divided by -1
			dstreg(t2).i /= sext16(immed(t2));
		break;
	default:
		interrupt(st, InvalidInstruction);
		return;
	}
}
void mod(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
			dstreg(t1).u %= srcreg(t1).u;
			break;
		case 0b01:
			dstreg(t1).i %= srcreg(t1).i;
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
		break;
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
			dstreg(t2).u %= immed(t2);
			break;
		case 0b01:
			dstreg(t2).i %= sext16(immed(t2));
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}

void mov(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		dstreg(t1) = srcreg(t1);
		break;
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
			dstreg(t2).u = immed(t2);
			break;
		case 0b01:
			dstreg(t2).i = sext16(immed(t2));
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void ld(State* st, Instruction inst)
{
	static const uint8_t sztable[] = { 1, 2, 4, 8 };
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		if (srcreg(t1).u >= st->mem_size - sztable[config(t1)])
			interrupt(st, PageFault);
		else if (inst.t1.extra & (1 << 10))
		{
			switch (config(t1))
			{
			case 0b00:
				dstreg(t1).i = *(int8_t*)(st->memory + srcreg(t1).u);
				break;
			case 0b01:
				dstreg(t1).i = *(int16_t*)(st->memory + srcreg(t1).u);
				break;
			case 0b10:
				dstreg(t1).i = *(int32_t*)(st->memory + srcreg(t1).u);
				break;
			case 0b11:
				dstreg(t1).i = *(int64_t*)(st->memory + srcreg(t1).u);
				break;
			}
		}
		else
		{
			switch (config(t1))
			{
			case 0b00:
				dstreg(t1).u = *(uint8_t*)(st->memory + srcreg(t1).u);
				break;
			case 0b01:
				dstreg(t1).u = *(uint16_t*)(st->memory + srcreg(t1).u);
				break;
			case 0b10:
				dstreg(t1).u = *(uint32_t*)(st->memory + srcreg(t1).u);
				break;
			case 0b11:
				dstreg(t1).u = *(uint64_t*)(st->memory + srcreg(t1).u);
				break;
			}
		}
		break;
	case 0b01:
		if (immed(t2) >= (sizeof(st->stat_regs) / sizeof(uint64_t)))
			interrupt(st, GeneralProtectionFault);
		else
			dstreg(t2).u = st->stat_regs[immed(t2)];
		break;
	default:
		interrupt(st, InvalidInstruction);
		return;
	}
}
void st(State* st, Instruction inst)
{
	static const uint8_t sztable[] = { 1, 2, 4, 8 };
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		if (dstreg(t1).u >= st->mem_size - sztable[config(t1)])
			interrupt(st, PageFault);
		else if (inst.t1.extra & (1 << 10))
		{
			switch (config(t1))
			{
			case 0b00:
				*(int8_t*)(st->memory + srcreg(t1).u) = (int8_t)dstreg(t1).i;
				break;
			case 0b01:
				*(int16_t*)(st->memory + srcreg(t1).u) = (int16_t)dstreg(t1).i;
				break;
			case 0b10:
				*(int32_t*)(st->memory + srcreg(t1).u) = (int32_t)dstreg(t1).i;
				break;
			case 0b11:
				*(int64_t*)(st->memory + srcreg(t1).u) = (int64_t)dstreg(t1).i;
				break;
			}
		}
		else
		{
			switch (config(t1))
			{
			case 0b00:
				*(uint8_t*)(st->memory + srcreg(t1).u) = (uint8_t)dstreg(t1).u;
				break;
			case 0b01:
				*(uint16_t*)(st->memory + srcreg(t1).u) = (uint16_t)dstreg(t1).u;
				break;
			case 0b10:
				*(uint32_t*)(st->memory + srcreg(t1).u) = (uint32_t)dstreg(t1).u;
				break;
			case 0b11:
				*(uint64_t*)(st->memory + srcreg(t1).u) = (uint64_t)dstreg(t1).u;
				break;
			}
		}
		break;
	case 0b01:
		if (immed(t2) >= (sizeof(st->stat_regs) / sizeof(uint64_t)))
			interrupt(st, GeneralProtectionFault);
		else
			st->stat_regs[immed(t2)] = dstreg(t2).u;
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}

void and(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		dstreg(t1).u &= srcreg(t1).u;
		if (config(t1) == 0b10)
			dstreg(t1).f.p = 0;
		break;
	case 0b10:
		if (config(t1) & 0b10) // if config(t1) is not equal to 00 or 01
			interrupt(st, InvalidInstruction);
		else
			dstreg(t2).u &= immed(t2);
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void or (State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		dstreg(t1).u |= srcreg(t1).u;
		if (config(t1) == 0b10)
			dstreg(t1).f.p = 0;
		break;
	case 0b10:
		if (config(t1) & 0b10) // if config(t1) is not equal to 00 or 01
			interrupt(st, InvalidInstruction);
		else
			dstreg(t2).u |= immed(t2);
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void xor(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		dstreg(t1).u ^= srcreg(t1).u;
		if (config(t1) == 0b10)
			dstreg(t1).f.p = 0;
		break;
	case 0b10:
		if (config(t1) & 0b10) // if config(t1) is not equal to 00 or 01
			interrupt(st, InvalidInstruction);
		else
			dstreg(t2).u ^= immed(t2);
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void not(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		dstreg(t1).u = ~srcreg(t1).u;
		if (config(t1) == 0b10)
			dstreg(t1).f.p = 0;
		break;
	case 0b10:
		if (config(t1) & 0b10) // if config(t1) is not equal to 00 or 01
			interrupt(st, InvalidInstruction);
		else
			dstreg(t2).u = ~immed(t2);
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void neg(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
		case 0b01:
			dstreg(t1).i = -srcreg(t1).i;
			break;
		case 0b10:
			dstreg(t1).f.v = -srcreg(t1).f.v;
			dstreg(t1).f.p = 0;
			break;
		case 0b11:
			dstreg(t1).d = -srcreg(t1).d;
			break;
		}
		break;
	case 0b11:
		switch (config(t2))
		{
		case 0b00:
			dstreg(t2).i = -(int64_t)immed(t2);
			break;
		case 0b10:
			dstreg(t2).i = -sext16(immed(t2));
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void shl(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
			setFlag(CF, srcreg(t1).u != 0 && (dstreg(t1).u & (1ull << (64 - (srcreg(t1).u & 0x3F)))));
			dstreg(t1).u <<= srcreg(t1).u & 0x3F;
			break;
		case 0b01:
			setFlag(CF, srcreg(t1).u != 0 && (dstreg(t1).u & (1ull << (63 - (srcreg(t1).u) & 0x3F))));
			dstreg(t1).i <<= immed(t2) & 0x3F;
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
			setFlag(CF, immed(t2) != 0 && (dstreg(t2).u & (1ull << (64 - (immed(t2) & 0x3F)))));
			dstreg(t2).u <<= (immed(t2) & 0x3f);
			break;
		case 0b01:
			setFlag(CF, immed(t2) != 0 && (dstreg(t2).u & (1ull << (63 - (immed(t2) & 0x3F)))));
			dstreg(t2).i <<= (immed(t2) & 0x3f);
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}

	}
}
void shr(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
			setFlag(CF, srcreg(t1).u != 0 && (dstreg(t1).u & (1ull << (srcreg(t1).u & 0x3F))));
			dstreg(t1).u >>= srcreg(t1).u & 0x3F;
			break;
		case 0b01:
			setFlag(CF, srcreg(t1).u != 0 && (dstreg(t1).u & (1ull << (srcreg(t1).u) & 0x3F)));
			dstreg(t1).i >>= immed(t2) & 0x3F;
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
			setFlag(CF, immed(t2) != 0 && (dstreg(t2).u & (1ull << (immed(t2) & 0x3F))));
			dstreg(t2).u >>= (immed(t2) & 0x3f);
			break;
		case 0b01:
			setFlag(CF, immed(t2) != 0 && (dstreg(t2).u & (1ull << (immed(t2) & 0x3F))));
			dstreg(t2).i >>= (immed(t2) & 0x3f);
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}

	}
}

void jmp(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		st->stat_regs[1] = dstreg(t1).u;
		break;
	case 0b01:
		st->stat_regs[1] = (uint64_t)(dstreg(t2).u + sext16(immed(t2)) * 4);
		break;
	case 0b10:
		st->stat_regs[1] = (uint64_t)(dstreg(t2).u + sext19(immed(t2)) * 4);
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void jx_base(State* st, uint64_t dest, uint64_t jmp_type)
{
	switch (jmp_type)
	{
	case JZ:
		if ((st->stat_regs[0] & ZF) == 0)
			st->stat_regs[1] = dest;
		break;
	case JNZ:
		if (st->stat_regs[0] & ZF)
			st->stat_regs[1] = dest;
		break;
	case JO:
		if (st->stat_regs[0] & OF)
			st->stat_regs[1] = dest;
		break;
	case JNO:
		if ((st->stat_regs[0] & OF) == 0)
			st->stat_regs[1] = dest;
		break;
	case JC:
		if (st->stat_regs[0] & CF)
			st->stat_regs[1] = dest;
		break;
	case JNC:
		if ((st->stat_regs[0] & CF) == 0)
			st->stat_regs[1] = dest;
		break;
	case JS:
		if (st->stat_regs[0] & SF)
			st->stat_regs[1] = dest;
		break;
	case JNS:
		if ((st->stat_regs[0] & SF) == 0)
			st->stat_regs[1] = dest;
		break;
	case JA:
		if ((st->stat_regs[0] & (ZF | CF)) == 0)
			st->stat_regs[1] = dest;
		break;
	case JG:
		if ((st->stat_regs[0] & (ZF | SF)) == 0)
			st->stat_regs[1] = dest;
		break;
	case JBE:
		if (st->stat_regs[0] & (ZF | CF))
			st->stat_regs[1] = dest;
		break;
	case JLE:
		if (st->stat_regs[0] & (ZF | SF))
			st->stat_regs[1] = dest;
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}
void jx(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b01:
		jx_base(st, dstreg(t2).u, immed(t2));
		break;
	case 0b10:
		jx_base(st, st->stat_regs[1] + sext19(immed(t3)), config(t3));
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}

void int_op(State* st, Instruction inst)
{
	if (inst.cmn.inst_t != 0b10)
		interrupt(st, InvalidInstruction);
	else
		interrupt(st, inst.t3.immediate);
}

void cmp(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (config(t1))
		{
		case 0b00:
		{
			uint64_t result;
			setFlag(CF, CHECKED_USUB(dstreg(t1).u, srcreg(t1).u, &result));
			setFlag(SF, false);
			setFlag(ZF, result == 0);
			setFlag(OF, false);
		}
		case 0b01:
		{
			int64_t result;
			setFlag(CF, false);
			setFlag(OF, CHECKED_SSUB(dstreg(t1).i, srcreg(t1).i, &result));
			setFlag(ZF, result == 0);
			setFlag(SF, result < 0);
		}
		case 0b10:
		{
			float result = dstreg(t1).f.v - srcreg(t1).f.v;
			setFlag(ZF, result == 0.0f);
			setFlag(SF, result < 0.0f);
			setFlag(CF, false);
			setFlag(OF, false);
		}
		case 0b11:
		{
			double result = dstreg(t1).d - srcreg(t1).d;
			setFlag(ZF, result == 0.0);
			setFlag(SF, result < 0.0);
			setFlag(CF, false);
			setFlag(OF, false);
		}
		}
		break;
	case 0b01:
		switch (config(t2))
		{
		case 0b00:
		{
			uint64_t result;
			setFlag(CF, CHECKED_USUB(dstreg(t2).u, immed(t2), &result));
			setFlag(SF, false);
			setFlag(ZF, result == 0);
			setFlag(OF, false);
		}
		case 0b01:
		{
			int64_t result;
			setFlag(CF, false);
			setFlag(OF, CHECKED_SSUB(dstreg(t2).i, sext16(immed(t2)), &result));
			setFlag(ZF, result == 0);
			setFlag(SF, result < 0);
		}
		default:
			interrupt(st, InvalidInstruction);
		}
		break;
	default:
		interrupt(st, InvalidInstruction);
	}
}

void fconv(State* st, Instruction inst)
{
	switch (inst.cmn.inst_t)
	{
	case 0b00:
		switch (inst.t1.extra)
		{
		case 0x0:
			dstreg(t1).f.v = (float)srcreg(t1).u;
			break;
		case 0x1:
			dstreg(t1).d = (float)srcreg(t1).u;
			break;
		case 0x2:
			dstreg(t1).f.v = (float)srcreg(t1).i;
			dstreg(t1).f.p = 0;
			break;
		case 0x3:
			dstreg(t1).d = (float)srcreg(t1).i;
			break;
		case 0x4:
			dstreg(t1).u = (uint64_t)srcreg(t1).d;
			break;
		case 0x5:
			dstreg(t1).i = (int64_t)srcreg(t1).d;
			break;
		case 0x6:
			dstreg(t1).f.v = (float)srcreg(t1).d;
			dstreg(t1).f.p = 0;
			break;
		case 0x7:
			dstreg(t1).u = (uint64_t)srcreg(t1).f.v;
			break;
		case 0x8:
			dstreg(t1).i = (int64_t)srcreg(t1).f.v;
			break;
		case 0x9:
			dstreg(t1).d = (double)srcreg(t1).f.v;
			break;
		default:
			interrupt(st, InvalidInstruction);
			break;
		}
		break;
	default:
		interrupt(st, InvalidInstruction);
		break;
	}
}

void syscall(State* st, Instruction inst)
{
	if (inst.cmn.inst_t == 0b10)
	{
		if (immed(t3) > (sizeof(st->syscalls) / sizeof(syscall_func)) - 1)
			interrupt(st, GeneralProtectionFault);
		else
		{
			syscall_func func = st->syscalls[immed(t3)];

			if (func == NULL)
				interrupt(st, GeneralProtectionFault);
			else
			{
				func(st);
			}
		}
	}
}

void execute(State* st_)
{
	if (st_->halt)
		return;

	Instruction inst;

	if (st_->stat_regs[1] % 4 != 0)
	{
		interrupt(st_, GeneralProtectionFault);
		return;
	}
	if (st_->stat_regs[1] >= st_->mem_size - 4) // Check if within physical memory
	{
		interrupt(st_, PageFault);
		return;
	}

	memcpy(&inst, st_->memory + st_->stat_regs[1], sizeof(Instruction));

	st_->stat_regs[1] += 4;

	switch (inst.cmn.opcode)
	{
	case NOP:
		break;
	case INT:
		int_op(st_, inst);
		break;
	case SYSCALL:
		syscall(st_, inst);
		break;
	case ADD:
		add(st_, inst);
		break;
	case SUB:
		sub(st_, inst);
		break;
	case MUL:
		mul(st_, inst);
		break;
	case DIV:
		div(st_, inst);
		break;
	case MOD:
		mod(st_, inst);
		break;
	case MOV:
		mov(st_, inst);
		break;
	case ST:
		st(st_, inst);
		break;
	case LD:
		ld(st_, inst);
		break;
	case AND:
		and (st_, inst);
		break;
	case OR:
		or (st_, inst);
		break;
	case XOR:
		xor (st_, inst);
		break;
	case NOT:
		not(st_, inst);
		break;
	case SHL:
		shl(st_, inst);
		break;
	case SHR:
		shr(st_, inst);
		break;
	case HALT:
		halt(st_);
		break;
	case JMP:
		jmp(st_, inst);
		break;
	case Jx:
		jx(st_, inst);
		break;
	case CMP:
		cmp(st_, inst);
		break;
	case FCONV:
		fconv(st_, inst);
		break;
	default:
		interrupt(st_, InvalidInstruction);
		break;
	}
}
