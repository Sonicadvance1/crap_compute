#pragma once

#include <epoxy/gl.h>
#include <string>

namespace GLUtils
{
	bool CheckShaderStatus(GLuint shader, std::string type, const char* src);
	bool CheckProgramLinkStatus(GLuint pgm);

}
