#pragma once

#include "DecodeTypes.h"

template<bool SSE>
void DecodeOnCPU(uint32_t* dst, uint8_t* src, int width, int height, TexType type);

