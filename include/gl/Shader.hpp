#ifndef SHADER_H_
#define SHADER_H_

#include <ostream>
#include <sstream>

#include <GL/glew.h>

#include "OpenGl.hpp"

#include "Types.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

inline std::string getShaderErrorMessage(const GLuint shader)
{
	GLint infoLogLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
	
	if (infoLogLength == 0)
	{
		return std::string();
	}
	
	GLchar* strInfoLog = new GLchar[infoLogLength];
	glGetShaderInfoLog(shader, infoLogLength, nullptr, strInfoLog);

	std::stringstream message;
	message << strInfoLog;

	delete[] strInfoLog;
	
	return message.str();
}

template <typename T>
class Shader
{
public:
	Shader() = default;

	explicit Shader(const GLuint id) : id_(id)
	{
	}
	
	explicit Shader(const std::string& source)
	{
		compile(source);
	}
	
	Shader(const Shader& other) = delete;
	
	Shader(Shader&& other)
	{
		this->id_ = other.id_;
		
		other.id_ = INVALID_ID;
	}
	
	operator GLuint() const
	{
		return id_;
	}
	
	Shader& operator=(const Shader& other) = delete;
	Shader& operator=(Shader&& other) = default;
	
	void compile(const std::string& source)
	{
		if (valid())				throw std::runtime_error("Cannot compile shader - shader must be destroyed first.");
		if (type() == INVALID_TYPE)	throw std::runtime_error("Cannot compile shader - shader type is not valid.");
		
		id_ = glCreateShader(type());
		
		if (id_ == INVALID_ID)
		{
			throw std::runtime_error("Could not create shader.");
		}
		
		try
		{
			const char* s = source.c_str();
			glShaderSource(id_, 1, &s, nullptr);
			glCompileShader(id_);
			
			GLint compiled = GL_FALSE;
			glGetShaderiv(id_, GL_COMPILE_STATUS, &compiled);
			
			if (!compiled)
			{
				std::stringstream message;
				message << "Could not compile shader: ";
				message << "\n" << getShaderErrorMessage(id_);
		
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
	
	void destroy()
	{
		if (!valid()) throw std::runtime_error("Cannot destroy shader - shader was not compiled.");
		
		glDeleteShader(id_);
		
		id_ = INVALID_ID;
	}

	GLuint id() const
	{
		return id_;
	}
	
	GLenum type() const
	{
		return static_cast<const T*>(this)->type();
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
		os << "Id: " << other.id_ << ", Type: " << other.type();
		return os;
	}

	static constexpr GLuint INVALID_ID = 0;
	static constexpr GLenum INVALID_TYPE = 0;

protected:
	~Shader()
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

#endif /* SHADER_H_ */

