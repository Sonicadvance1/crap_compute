#include "Sampler.h"

Sampler::Sampler()
{
	glGenSamplers(1, &mSampler);
	glSamplerParameteri(mSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(mSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

Sampler::~Sampler()
{
	glDeleteSamplers(1, &mSampler);
}

void Sampler::BindSampler(int stage)
{
	 glBindSampler(stage, mSampler);
}
