#pragma once
#include <epoxy/gl.h>

class Sampler
{
public:
	Sampler();
	void BindSampler(int stage);
	~Sampler();
private:
	GLuint mSampler;
};
