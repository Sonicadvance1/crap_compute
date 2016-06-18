// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DecodeTypes.h"

#include <stdint.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#if defined _M_GENERIC
#  define _M_SSE 0
#elif _MSC_VER || __INTEL_COMPILER
#  define _M_SSE 0x402
#elif defined __GNUC__
# if defined __SSE4_2__
#  define _M_SSE 0x402
# elif defined __SSE4_1__
#  define _M_SSE 0x401
# elif defined __SSSE3__
#  define _M_SSE 0x301
# elif defined __SSE3__
#  define _M_SSE 0x300
# endif
#endif

void DecodeOnCPU_SSE(uint32_t* dst, uint8_t* src, int width, int height, TexType type)
{
	const int Wsteps4 = (width + 3) / 4;
	const int Wsteps8 = (width + 7) / 8;

	switch(type)
	{
	case TexType::TYPE_RGB565:
		{
			// JSD optimized with SSE2 intrinsics.
			// Produces an ~78% speed improvement over reference C implementation.
			const __m128i kMaskR0 = _mm_set1_epi32(0x000000F8);
			const __m128i kMaskG0 = _mm_set1_epi32(0x0000FC00);
			const __m128i kMaskG1 = _mm_set1_epi32(0x00000300);
			const __m128i kMaskB0 = _mm_set1_epi32(0x00F80000);
			const __m128i kAlpha  = _mm_set1_epi32(0xFF000000);
			for (int y = 0; y < height; y += 4)
				for (int x = 0, yStep = (y / 4) * Wsteps4; x < width; x += 4, yStep++)
					for (int iy = 0, xStep = 4 * yStep; iy < 4; iy++, xStep++)
					{
						__m128i *dxtsrc = (__m128i *)(src + 8 * xStep);
						// Load 4x 16-bit colors: (0000 0000 hgfe dcba)
						// where hg, fe, ba, and dc are 16-bit colors in big-endian order
						const __m128i rgb565x4 = _mm_loadl_epi64(dxtsrc);

						// The big-endian 16-bit colors `ba` and `dc` look like 0b_gggBBBbb_RRRrrGGg in a little endian xmm register
						// Unpack `hgfe dcba` to `hhgg ffee ddcc bbaa`, where each 32-bit word is now 0b_gggBBBbb_RRRrrGGg_gggBBBbb_RRRrrGGg
						const __m128i c0 = _mm_unpacklo_epi16(rgb565x4, rgb565x4);

						// swizzle 0b_gggBBBbb_RRRrrGGg_gggBBBbb_RRRrrGGg
						//      to 0b_11111111_BBBbbBBB_GGggggGG_RRRrrRRR

						// 0b_gggBBBbb_RRRrrGGg_gggBBBbb_RRRrrGGg &
						// 0b_00000000_00000000_00000000_11111000 =
						// 0b_00000000_00000000_00000000_RRRrr000
						const __m128i r0 = _mm_and_si128(c0, kMaskR0);
						// 0b_00000000_00000000_00000000_RRRrr000 >> 5 [32] =
						// 0b_00000000_00000000_00000000_00000RRR
						const __m128i r1 = _mm_srli_epi32(r0, 5);

						// 0b_gggBBBbb_RRRrrGGg_gggBBBbb_RRRrrGGg >> 3 [32] =
						// 0b_000gggBB_BbbRRRrr_GGggggBB_BbbRRRrr &
						// 0b_00000000_00000000_11111100_00000000 =
						// 0b_00000000_00000000_GGgggg00_00000000
						const __m128i gtmp = _mm_srli_epi32(c0, 3);
						const __m128i g0 = _mm_and_si128(gtmp, kMaskG0);
						// 0b_GGggggBB_BbbRRRrr_GGggggBB_Bbb00000 >> 6 [32] =
						// 0b_000000GG_ggggBBBb_bRRRrrGG_ggggBBBb &
						// 0b_00000000_00000000_00000011_00000000 =
						// 0b_00000000_00000000_000000GG_00000000 =
						const __m128i g1 = _mm_and_si128(_mm_srli_epi32(gtmp, 6), kMaskG1);

						// 0b_gggBBBbb_RRRrrGGg_gggBBBbb_RRRrrGGg >> 5 [32] =
						// 0b_00000ggg_BBBbbRRR_rrGGgggg_BBBbbRRR &
						// 0b_00000000_11111000_00000000_00000000 =
						// 0b_00000000_BBBbb000_00000000_00000000
						const __m128i b0 = _mm_and_si128(_mm_srli_epi32(c0, 5), kMaskB0);
						// 0b_00000000_BBBbb000_00000000_00000000 >> 5 [16] =
						// 0b_00000000_00000BBB_00000000_00000000
						const __m128i b1 = _mm_srli_epi16(b0, 5);

						// OR together the final RGB bits and the alpha component:
						const __m128i abgr888x4 = _mm_or_si128(
							_mm_or_si128(
								_mm_or_si128(r0, r1),
								_mm_or_si128(g0, g1)
							),
							_mm_or_si128(
								_mm_or_si128(b0, b1),
								kAlpha
							)
						);

						__m128i *ptr = (__m128i *)(dst + (y + iy) * width + x);
						_mm_storeu_si128(ptr, abgr888x4);
					}
		}

	break;
	}
}

constexpr uint8_t Convert3To8(uint8_t v)
{
	// Swizzle bits: 00000123 -> 12312312
	return (v << 5) | (v << 2) | (v >> 1);
}

constexpr uint8_t Convert4To8(uint8_t v)
{
	// Swizzle bits: 00001234 -> 12341234
	return (v << 4) | v;
}

constexpr uint8_t Convert5To8(uint8_t v)
{
	// Swizzle bits: 00012345 -> 12345123
	return (v << 3) | (v >> 2);
}

constexpr uint8_t Convert6To8(uint8_t v)
{
	// Swizzle bits: 00123456 -> 12345612
	return (v << 2) | (v >> 4);
}
static inline uint32_t DecodePixel_RGB565(uint16_t val)
{
	int r,g,b,a;
	r=Convert5To8((val>>11) & 0x1f);
	g=Convert6To8((val>>5 ) & 0x3f);
	b=Convert5To8((val    ) & 0x1f);
	a=0xFF;
	return  r | (g<<8) | (b << 16) | (a << 24);
}
#include <byteswap.h>

inline uint16_t swap16(uint16_t _data) {return bswap_16(_data);}
inline uint32_t swap32(uint32_t _data) {return bswap_32(_data);}
inline uint64_t swap64(uint64_t _data) {return bswap_64(_data);}

template<bool SSE>
void DecodeOnCPU(uint32_t* dst, uint8_t* src, int width, int height, TexType type)
{
	if (SSE)
	{
		DecodeOnCPU_SSE(dst, src, width, height, type);
		return;
	}

	const int Wsteps4 = (width + 3) / 4;
	const int Wsteps8 = (width + 7) / 8;

	switch(type)
	{
	case TexType::TYPE_RGB565:
		// Reference C implementation.
		for (int y = 0; y < height; y += 4)
			for (int x = 0; x < width; x += 4)
				for (int iy = 0; iy < 4; iy++, src += 8)
				{
					uint32_t *ptr = dst + (y + iy) * width + x;
					uint16_t *s = (uint16_t *)src;
					for (int j = 0; j < 4; j++)
						*ptr++ = DecodePixel_RGB565(swap16(*s++));
				}
	break;
	}
}

template void DecodeOnCPU<true>(uint32_t*, uint8_t*, int, int, TexType);
template void DecodeOnCPU<false>(uint32_t*, uint8_t*, int, int, TexType);
