#include <map>
#include <sstream>

#include "CPUDecoder.h"
#include "GLUtils.h"
#include "GPUDecoder.h"

std::string GenHeader(TexType type)
{
	std::ostringstream output;
	output <<
	"#version 320 es\n"
	"precision highp uimageBuffer;\n"
	"precision highp uimage2D;\n"
	"precision highp usamplerBuffer;\n"

	"uint Convert5To8(uint val)\n"
	"{\n"
		"\treturn (val << 3) | (val >> 2);\n"
	"}\n\n"

	"uint Convert6To8(uint val)\n"
	"{\n"
		"\treturn (val << 2) | (val >> 4);\n"
	"}\n\n"

	"uint bswap16(uint src)\n"
	"{\n"
	"	return ((src & 0xFFu) << 8u) | (src >> 8u);\n"
	"}\n";


	return output.str();
}

std::map<TexType, GLuint> s_pgms;

#define RGB565_DIVISOR 4

GLuint GenerateDecoderProgram(TexType type)
{
	auto it = s_pgms.find(type);
	if (it != s_pgms.end())
		return it->second;

	std::string cs_src;
	cs_src += GenHeader(type);
	switch(type)
	{
	case TexType::TYPE_RGB565:
	{
		const char* cs_test =
		"#define RGB565_DIVISOR %d\n"
		"layout(local_size_x = 4, local_size_y = 4) in;\n"
//		"layout(rgba16ui, binding = 0) readonly uniform uimageBuffer enc_tex;\n"
		"layout(rgba8ui, binding = 1) writeonly uniform uimage2D dec_tex;\n"
		"layout(binding = 9) uniform usamplerBuffer enc_buf;\n"

		"uvec4 LoadTexel(ivec2 dim, ivec2 loc)\n"
		"{\n"
			// X and Y are provided in regular linear x/y coordinates
			// Source coordinates are Width * y + x
			// Workgroup size is width / 4
			// So source is (imageDim * (y / 2)) + (x / 2)
			// enc_tex has two pixels per u32 so we need to
			// Each texture load loads a u32
			// Src 0 = (0,0) & (0, 1)
			// Src 1 = (0,2) & (0x3)
			// SRCDIM | offset
			// ---------------
			// (0, 0) | 0 (r)
			// (0, 1) | 0 (g)
			// (0, 2) | 0 (b)
			// (0, 3) | 0 (a)
			// (1, 0) | (32*1 / 2)+0/2 = 16
			// (1, 1) | 16
			// (1, 2) | 17
			"\tint srcloc = ((dim.x * loc.y) >> 2) + (loc.x >> 2);\n"
			//"\tuvec4 col0 = imageLoad(enc_tex, srcloc);\n"
			"\tuvec4 col0 = texelFetch(enc_buf, srcloc);\n"
			"\tcol0[0] = bswap16(col0[0]);\n"
			"\tcol0[1] = bswap16(col0[1]);\n"
			"\tcol0[2] = bswap16(col0[2]);\n"
			"\tcol0[3] = bswap16(col0[3]);\n"
			"\treturn col0;\n"
		"}\n\n"

		"// RGB565\n"
		"void main() {\n"
		"	ivec2 start = ivec2(gl_WorkGroupID.xy)* RGB565_DIVISOR;\n"
		"	ivec2 dims = ivec2(gl_NumWorkGroups.xy)* RGB565_DIVISOR;\n"
		"	uvec4 in_col[4];\n"
#if 1
		"	in_col[0] = LoadTexel(dims, start + ivec2(0, 0));\n"
		"	in_col[1] = LoadTexel(dims, start + ivec2(0, 1));\n"
		"	in_col[2] = LoadTexel(dims, start + ivec2(0, 2));\n"
		"	in_col[3] = LoadTexel(dims, start + ivec2(0, 3));\n"
#else
		"	in_col[1] = in_col[2] = in_col[3] = in_col[0] = uvec4(0);\n"
#endif
		"	for (int y = 0; y < 4; ++y)\n"
		"	{\n"
		"		for (int x = 0; x < 4; ++x)\n"
		"		{\n"
		"			uvec4 out_col;\n"
		"			out_col.r = Convert5To8((in_col[y][x] >> 11u) & 0x1Fu);\n"
		"			out_col.g = Convert6To8((in_col[y][x] >>  5u) & 0x3Fu);\n"
		"			out_col.b = Convert5To8(in_col[y][x]          & 0x1Fu);\n"
		"			out_col.a = 0xFFu;\n"
		"			imageStore(dec_tex, start + ivec2(x, y), out_col);\n"
		"		}\n"
		"	}\n"
		"}\n";

		//for (int y = 0; y < height; y += 4)
		//	for (int x = 0; x < width; x += 4)
		//		for (int iy = 0; iy < 4; iy++, src += 8)
		//		{
		//			uint32_t *ptr = dst + (y + iy) * width + x;
		//			uint16_t *s = (uint16_t *)src;
		//			for (int j = 0; j < 4; j++)
		//				*ptr++ = DecodePixel_RGB565(swap16(*s++));
		//		}

		char tmp[2048];
		sprintf(tmp, cs_test, RGB565_DIVISOR);
		cs_src += tmp;
		GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
		GLuint cs_pgm = glCreateProgram();

		std::array<const char*, 1> srcs = {
			cs_src.c_str(),
		};
		glShaderSource(cs, 1, &srcs[0], NULL);

		glCompileShader(cs);

		GLUtils::CheckShaderStatus(cs, "cs", cs_src.c_str());

		glAttachShader(cs_pgm, cs);
		glLinkProgram(cs_pgm);

		GLUtils::CheckProgramLinkStatus(cs_pgm);
		s_pgms[type] = cs_pgm;
		return cs_pgm;
	}

	break;
	}
}

void DispatchType(TexType type, int w, int h)
{
	switch(type)
	{
	case TexType::TYPE_RGB565:
		glDispatchCompute(w / RGB565_DIVISOR, h / RGB565_DIVISOR, 1);
	break;
	}
}

TextureConvert::TextureConvert(TexType type, int w, int h)
	: m_type(type), m_w(w), m_h(h)
{
	GLuint imgs[2];

	glGenTextures(2, imgs);
	glGenBuffers(1, &enc_buf);
	enc_img = imgs[0];
	dec_img = imgs[1];

	printf("Creating texture\n");
	// Encoded image
	glBindTexture(GL_TEXTURE_BUFFER, enc_img);
	glBindBuffer(GL_TEXTURE_BUFFER, enc_buf);

	// 8 bits per component
	// 4 components per colour
	// But since we are doing RGB565 it will be 16bits per colour
	// So there will be two colours per texel fetch
	data.resize(m_w * m_h * 2);
	cpudata.resize(m_w * m_h * 4);
	GenRGB565();
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA16UI, enc_buf);

	// Decoded image
	glBindTexture(GL_TEXTURE_2D, dec_img);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// 8 bits per component
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8UI, m_w, m_h);
	printf("Done creating\n");

	m_cputime.Start();
	m_avgtime.Start();
}

void TextureConvert::GenRGB565()
{
	uint64_t time = m_cputime.End() / 1000;
	if (time >= 2000)
	{
		m_cputime.Start();

		m_shift_val <<= 1;
		if (m_shift_val > m_w)
			m_shift_val = 1;

		for (int y = 0; y < m_h; ++y)
			for (int x = 0; x < m_w; ++x)
			{
				int i = (y * m_w + x) * 2;
				if (x & m_shift_val)
					*(uint16_t*)&data[i] = 0xE0FF;
				else
					*(uint16_t*)&data[i] = 0xFF07;

			}

		glBindBuffer(GL_TEXTURE_BUFFER, enc_buf);
		glBufferData(GL_TEXTURE_BUFFER, data.size(), &data[0], GL_STREAM_DRAW);
	}
}

void TextureConvert::DecodeImage()
{
	int64_t time1, time2, time3, time4;
	GenRGB565();
	glBindImageTexture(0, enc_img, 0, false, 0, GL_READ_ONLY, GL_RGBA16UI);
	glBindImageTexture(1, dec_img, 0, false, 0, GL_WRITE_ONLY, GL_RGBA8UI);

	GLuint pgm = GenerateDecoderProgram(m_type);
	glUseProgram(pgm);


	mSampler.BindSampler(9);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_BUFFER, enc_img);

	m_timer.BeginTimer();
	DispatchType(m_type, m_w, m_h);
	m_timer.EndTimer();

	time1 = CPUTimer::GetTime();
		DecodeOnCPU<false>(&cpudata[0], &data[0], m_w, m_h, m_type);
	time2 = CPUTimer::GetTime();

	time3 = CPUTimer::GetTime();
		DecodeOnCPU<true>(&cpudata[0], &data[0], m_w, m_h, m_type);
	time4 = CPUTimer::GetTime();


	uint64_t time = m_timer.GetTime();

	num_times++;
	totaltime_gpu += time;
	totaltime_cpu += (time2 - time1);
	totaltime_cpusse += (time4 - time3);
	uint64_t total_avg = m_avgtime.End();

	if (total_avg >= (1000 * 1000))
	{
		printf("Compute shader took: %ldus(%ldms) GPU time (%ldus(%ldms) CPU time) (%ldus(%ldms) SSE CPU time) %ld runs in %ldms\n",
			(totaltime_gpu / num_times) / 1000, (totaltime_gpu / num_times) / 1000 / 1000,
			(totaltime_cpu / num_times), (totaltime_cpu / num_times) / 1000,
			(totaltime_cpusse / num_times), (totaltime_cpusse / num_times) / 1000,
			num_times, total_avg / 1000);

		num_times = 0;
		totaltime_gpu = totaltime_cpu = totaltime_cpusse = 0;
		m_avgtime.Start();
	}
}

