#include "Test.hpp"
#include "rlm3-helper.h"
#include "stm32f4xx.h"


#define AS_STRING(...) #__VA_ARGS__
#define COUNT_ARGS2(...) AS_STRING(ignore, ## __VA_ARGS__, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

TEST_CASE(COUNT_ARGS_HappyCase)
{
//	ASSERT(COUNT_ARGS() == 0); // Does not work with ARM compiler.
	ASSERT(COUNT_ARGS(a1) == 1);
	ASSERT(COUNT_ARGS(a1, a2) == 2);
	ASSERT(COUNT_ARGS(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30) == 30);
}

#define FLAG_A_Pos		0
#define FLAG_A_Msk		(0b0111 << FLAG_A_Pos)
#define FLAG_A			FLAG_A_Msk
#define FLAG_B_Pos		3
#define FLAG_B_Msk		(0b0011 << FLAG_B_Pos)
#define FLAG_B			FLAG_B_Msk
#define FLAG_C_Pos		5
#define FLAG_C_Msk		(0b0001 << FLAG_C_Pos)
#define FLAG_C			FLAG_C_Msk
#define FLAG_D_Pos		6
#define FLAG_D_Msk		(0b0001 << FLAG_D_Pos)
#define FLAG_D			FLAG_D_Msk
#define FLAG_E_Pos		7
#define FLAG_E_Msk		(0b0001 << FLAG_E_Pos)
#define FLAG_E			FLAG_E_Msk
#define FLAG_F_Pos		8
#define FLAG_F_Msk		(0b0011 << FLAG_F_Pos)
#define FLAG_F			FLAG_F_Msk
#define FLAG_G_Pos		10
#define FLAG_G_Msk		(0b0111 << FLAG_G_Pos)
#define FLAG_G			FLAG_G_Msk
#define FLAG_H_Pos		13
#define FLAG_H_Msk		(0b01111111 << FLAG_H_Pos)
#define FLAG_H			FLAG_H_Msk
#define FLAG_I_Pos		20
#define FLAG_I_Msk		(0b01111111 << FLAG_I_Pos)
#define FLAG_I			FLAG_I_Msk
#define FLAG_J_Pos		27
#define FLAG_J_Msk		(0b0111 << FLAG_J_Pos)
#define FLAG_J			FLAG_J_Msk


TEST_CASE(SET_REGISTER_FLAGS_SimpleSet)
{
	uint32_t reg = 0b10001000000000;

	SET_REGISTER_FLAGS(reg,
			FLAG(FLAG_G, 0b101));

	ASSERT(reg == 0b11011000000000);
}

TEST_CASE(SET_REGISTER_FLAGS_SimpleClear)
{
	uint32_t reg = 0b11111000000000;

	SET_REGISTER_FLAGS(reg,
			FLAG(FLAG_G, 0));

	ASSERT(reg == 0b10001000000000);
}

TEST_CASE(SET_REGISTER_FLAGS_SimpleModify)
{
	uint32_t reg = 0b11011000000000;

	SET_REGISTER_FLAGS(reg,
			FLAG(FLAG_G, 0b0010));

	ASSERT(reg == 0b10101000000000);
}

