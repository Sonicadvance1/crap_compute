#pragma once

#include <ctime>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include <epoxy/gl.h>

class GPUTimer
{
public:
	GPUTimer()
	{
		glGenQueries(1, &m_query);
	}
	~GPUTimer()
	{
		glDeleteQueries(1, &m_query);
	}

	void BeginTimer()
	{
		glBeginQuery(GL_TIME_ELAPSED, m_query);
	}

	void EndTimer()
	{
		glEndQuery(GL_TIME_ELAPSED);
	}

	uint64_t GetTime()
	{
		uint64_t res = 0;
		glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, &res);
		return res;
	}
	static int64_t GetTimestamp()
	{
		int64_t res = 0;
		glGetInteger64v(GL_TIMESTAMP, &res);
		return res;
	}

private:
	GLuint m_query;
};

class CPUTimer
{
public:
	CPUTimer()
	{
	}

	void Start()
	{
		start = GetTime();
	}

	uint64_t End()
	{
		end = GetTime();
		return end - start;
	}

	static uint64_t GetTime()
	{
		struct timespec t;
		(void)clock_gettime(CLOCK_MONOTONIC, &t);
		return ((uint64_t)(t.tv_sec * 1000000 + t.tv_nsec / 1000));
	}
private:
	uint64_t start, end;
};

