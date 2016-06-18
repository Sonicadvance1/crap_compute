#include <array>
#include <map>
#include <memory>
#include <vector>

#include <epoxy/gl.h>

#include "Context.h"
#include "CPUDecoder.h"
#include "GLUtils.h"
#include "GPUDecoder.h"
#include "GPUTimer.h"

TextureConvert* conv;

void DrawTriangle(uint32_t TexDim)
{
	conv = new TextureConvert(TexType::TYPE_RGB565, TexDim, TexDim);

	const char* fs_test =
	"#version 310 es\n"
	"precision highp float;\n\n"
	"precision highp uimage2D;\n"

	"in vec4 vert;\n"
	"layout(rgba8ui, binding = 1) readonly uniform uimage2D image;\n"
	"layout(binding = 0) uniform sampler2D tex;\n"

	"out vec4 ocol;\n"
	"void main() {\n"
		"\tvec2 fcoords = vec2(0.0);\n"
		"\tfcoords = (gl_FragCoord.xy);\n"
		"\tivec2 coords = ivec2(fcoords);\n"
		"\tuvec4 ucol = imageLoad(image, coords);\n"
		"\tvec4 out_col = vec4(ucol);\n"
		"\tocol = vec4(out_col) / 255.0;\n"
	"}\n";

	const char* vs_test =
	"#version 310 es\n"

	"in vec4 pos;\n"
	"out vec4 vert;\n"

	"void main() {\n"
		"\tgl_Position = pos;\n"
		"\tvert = pos;\n"
	"}\n";

	GLuint fs, vs, pgm;
	GLint stat, attr_pos, attr_col;
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	vs = glCreateShader(GL_VERTEX_SHADER);
	pgm = glCreateProgram();

	glShaderSource(fs, 1, &fs_test, NULL);
	glShaderSource(vs, 1, &vs_test, NULL);

	glCompileShader(fs);
	glCompileShader(vs);

	GLUtils::CheckShaderStatus(fs, "fs", fs_test);
	GLUtils::CheckShaderStatus(vs, "vs", vs_test);

	glAttachShader(pgm, fs);
	glAttachShader(pgm, vs);
	glLinkProgram(pgm);

	GLUtils::CheckProgramLinkStatus(pgm);

	glUseProgram(pgm);

	// Get attribute locations
	attr_pos = glGetAttribLocation(pgm, "pos");

	glEnableVertexAttribArray(attr_pos);

	glClearColor(0.4, 0.4, 0.4, 0.0);

	const GLfloat verts[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1,
	};
	const GLfloat colors[] = {
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 1, 0, 1,
	};

	conv->DecodeImage();

	std::clock_t begin, start, end;
	int iters = 0;
	begin = std::clock();
	for (;;)
	{
		conv->DecodeImage();
		glUseProgram(pgm);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, conv->GetDecImg());

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		Context::Swap();
		end = std::clock();
		iters++;
		double duration = (end - begin) / (double)CLOCKS_PER_SEC;
		if (duration >= 1.0)
		{
			printf("iterated: %d\n", iters);
			iters = 0;
			begin = std::clock();
		}

	}
}

static void APIENTRY ErrorCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	printf("Message: '%s'\n", message);
	if (type == GL_DEBUG_TYPE_ERROR_ARB)
		__builtin_trap();
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <tex dim>\n", argv[0]);
		return 0 ;
	}
	uint32_t TexDim = atoi(argv[1]);
	GLint x,y,z;
	Context::Create();

	printf("Are we in desktop GL? %s\n", epoxy_is_desktop_gl() ? "Yes" : "No");
	printf("Our GL version %d\n", epoxy_gl_version());

	printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
	printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
	printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
	printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &x);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &y);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &z);
	printf("GL_MAX_COMPUTE_WORK_GROUP_SIZE = %d %d %d\n", x, y, z);

	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &x);
	printf("GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS = %d\n", x);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &x);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &y);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &z);
	printf("GL_MAX_COMPUTE_WORK_GROUP_COUNT = %d %d %d\n", x, y, z);



	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
	glDebugMessageCallback(ErrorCallback, nullptr);
	glEnable(GL_DEBUG_OUTPUT);

	DrawTriangle(TexDim);

	Context::Shutdown();
}
