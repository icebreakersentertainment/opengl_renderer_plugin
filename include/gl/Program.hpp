#ifndef PROGRAM_H_
#define PROGRAM_H_

#include <ostream>

#include <GL/glew.h>

#include "OpenGl.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

inline std::string getShaderProgramErrorMessage(const GLuint program)
{
	GLint infoLogLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
	
	if (infoLogLength == 0)
	{
		return std::string();
	}

	GLchar* strInfoLog = new GLchar[infoLogLength ];
	glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog);

	std::stringstream message;
	message << strInfoLog;

	delete[] strInfoLog;
	
	return message.str();
}

template <typename T>
class Program
{
public:
	Program() = default;

	explicit Program(const GLuint id) : id_(id)
	{
	}
	
	Program(const VertexShader& vertexShader, const FragmentShader& fragmentShader)
	{
		link(vertexShader, fragmentShader);
	}
	
	Program(
		const VertexShader& vertexShader,
		const TessellationControlShader& tessellationControlShader,
		const TessellationEvaluationShader& tessellationEvaluationShader,
		const FragmentShader& fragmentShader
	)
	{
		link(vertexShader, tessellationControlShader, tessellationEvaluationShader, fragmentShader);
	}
	
	Program(const Program& other) = delete;
	
	Program(Program&& other)
	{
		this->id_ = other.id_;
		
		other.id_ = INVALID_ID;
	}
	
	operator GLuint() const
	{
		return id_;
	}
	
	Program& operator=(const Program& other) = delete;
	Program& operator=(Program&& other) = default;
	
	void link(const VertexShader& vertexShader, const FragmentShader& fragmentShader)
	{
		if (valid()) throw std::runtime_error("Cannot link program - program must be destroyed first.");
		
		id_ = glCreateProgram();
		
		if (id_ == INVALID_ID)
		{
			throw std::runtime_error("Could not create program.");
		}
		
		try
		{
			glAttachShader(id_, vertexShader);
			glAttachShader(id_, fragmentShader);
			glLinkProgram(id_);
			
			GLint compiled = GL_FALSE;
			glGetProgramiv(id_, GL_LINK_STATUS, &compiled);
			
			if (!compiled)
			{
				std::stringstream message;
				message << "Could not link program: ";
				message << "\n" << getShaderProgramErrorMessage(id_);
				
				throw std::runtime_error(message.str());
			}
			
			ASSERT_GL_ERROR();
		}
		catch (const std::exception& e)
		{
			// Cleanup
			destroy();
			
			throw e;
		}
	}
	
	void link(
		const VertexShader& vertexShader,
		const TessellationControlShader& tessellationControlShader,
		const TessellationEvaluationShader& tessellationEvaluationShader,
		const FragmentShader& fragmentShader
	)
	{
		if (valid()) throw std::runtime_error("Cannot link program - program must be destroyed first.");
		
		id_ = glCreateProgram();
		
		if (id_ == INVALID_ID)
		{
			throw std::runtime_error("Could not create program.");
		}
		
		try
		{
			glAttachShader(id_, vertexShader);
			glAttachShader(id_, tessellationControlShader);
			glAttachShader(id_, tessellationEvaluationShader);
			glAttachShader(id_, fragmentShader);
			glLinkProgram(id_);
			
			GLint compiled = GL_FALSE;
			glGetProgramiv(id_, GL_LINK_STATUS, &compiled);
			
			if (!compiled)
			{
				std::stringstream message;
				message << "Could not link program: ";
				message << "\n" << getShaderProgramErrorMessage(id_);
				
				throw std::runtime_error(message.str());
			}
			
			ASSERT_GL_ERROR();
		}
		catch (const std::exception& e)
		{
			// Cleanup
			destroy();
			
			throw e;
		}
	}
	
	void use()
	{
		if (!valid()) throw std::runtime_error("Cannot use program - program must be linked first.");
		
		glUseProgram(id_);
	}
	
	void destroy()
	{
		if (!valid()) throw std::runtime_error("Cannot destroy program - program was not linked.");
		
		glDeleteProgram(id_);
		
		id_ = INVALID_ID;
	}

	GLuint id() const
	{
		return id_;
	}
		
	bool valid() const
	{
		return (id_ != INVALID_ID);
	}
	
	explicit operator bool() const
	{
		return valid();
	}
	
	bool operator==(const T& other) const
	{
		return id_ == other.id_;
	}

	bool operator!=(const T& other) const
	{
		return id_ != other.id_;
	}

	friend std::ostream& operator<<(std::ostream& os, const T& other)
	{
		os << "Id: " << other.id_;
		return os;
	}

	static constexpr GLuint INVALID_ID = 0;

protected:
	~Program()
	{
		if (valid())
		{
			destroy();
		}
	}

private:
	GLuint id_ = INVALID_ID;
};

}
}
}
}

#endif /* PROGRAM_H_ */

