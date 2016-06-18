#pragma once
#include "DecodeTypes.h"
#include "GPUTimer.h"
#include "Sampler.h"

#include <stdint.h>

class TextureConvert
{
public:
	TextureConvert(TexType type, int w, int h);

	void DecodeImage();

	GLuint GetEncImg() const { return enc_img; }
	GLuint GetDecImg() const { return dec_img; }

private:

	void GenRGB565();

	GLuint enc_img, dec_img;
	GLuint enc_buf;
	TexType m_type;
	int m_w, m_h;
	std::vector<uint8_t> data;
	std::vector<uint32_t> cpudata;
	uint32_t m_shift_val = 1;
	GPUTimer m_timer;
	CPUTimer m_cputime;
	Sampler mSampler;

	// Average time spent in shader
	CPUTimer m_avgtime;
	uint64_t totaltime_gpu = 0, totaltime_cpu = 0, totaltime_cpusse = 0, num_times = 0;
};
