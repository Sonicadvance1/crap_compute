#include <iostream>
#include <fstream>
#include <stdio.h>

#include "GLUtils.h"

namespace GLUtils
{
	bool CheckShaderStatus(GLuint shader, std::string type, const char* src)
	{
		GLint stat, compileStatus;
		GLsizei length = 0;

		glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
		if (!stat)
		{
			printf("Couldn't compile shader!\n");
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			if (compileStatus != GL_TRUE || length > 1)
			{
				GLsizei charsWritten;
				GLchar* infoLog = new GLchar[length];
				glGetShaderInfoLog(shader, length, &charsWritten, infoLog);
				printf("Error to compile: '%s'\n", infoLog);
				std::ofstream myfile;

				std::string filename = "bad_" + type;
				myfile.open (filename);
				myfile << src;
				myfile << infoLog;
				myfile.close();
				delete[] infoLog;
				exit(1);
			}

			return false;
		}

		std::ofstream myfile;

		std::string filename = "good_" + type;
		myfile.open (filename);
		myfile << src;
		myfile.close();

		return true;
	}

	bool CheckProgramLinkStatus(GLuint pgm)
	{
		GLint linkStatus;
		GLsizei length = 0;

		glGetProgramiv(pgm, GL_LINK_STATUS, &linkStatus);
		glGetProgramiv(pgm, GL_INFO_LOG_LENGTH, &length);
		if (linkStatus != GL_TRUE || length > 1)
		{
			GLsizei charsWritten;
			GLchar* infoLog = new GLchar[length];
			glGetProgramInfoLog(pgm, length, &charsWritten, infoLog);
			printf("Error to link: '%s'\n", infoLog);
			delete[] infoLog;
			return false;
		}
		return true;
	}
}
