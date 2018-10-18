#ifndef RENDERBUFFER_H_
#define RENDERBUFFER_H_

#include <ostream>

#include <GL/glew.h>

#include "OpenGl.hpp"
#include "Bindable.hpp"

#include "Types.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class RenderBuffer : public Bindable<RenderBuffer>
{
public:
	RenderBuffer(void) = default;

	explicit RenderBuffer(const GLuint id) : id_(id)
	{
	}
	
	RenderBuffer(const RenderBuffer& other) = delete;
	
	RenderBuffer(RenderBuffer&& other)
	{
		this->id_ = other.id_;
		
		other.id_ = INVALID_ID;
	}
	
	~RenderBuffer()
	{
		if (valid())
		{
			destroy();
		}
	}
	
	operator GLuint() const
	{
		return id_;
	}
	
	RenderBuffer& operator=(const RenderBuffer& other) = delete;
	RenderBuffer& operator=(RenderBuffer&& other) = default;
	
	void generate()
	{
		if (valid()) throw std::runtime_error("Cannot generate render buffer - render buffer was already created.");
		
		glGenRenderbuffers(1, &id_);
		
		if (id_ == INVALID_ID)
		{
			throw std::runtime_error("Could not create render buffer.");
		}
	}
	
	void setStorage(GLenum internalformat, GLsizei width, GLsizei height)
	{
		if (!valid()) throw std::runtime_error("Cannot set storage for render buffer - render buffer was not created.");
		
		bind();
		
		glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
	}
	
	void bind()
	{
		if (!valid()) throw std::runtime_error("Cannot bind render buffer - render buffer was not created.");
		
		glBindRenderbuffer(GL_RENDERBUFFER, id_);
	}
	
	void destroy()
	{
		if (!valid()) throw std::runtime_error("Cannot destroy render buffer - render buffer was not created.");
		
		glDeleteRenderbuffers(1, &id_);
		
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
	/*
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
*/
	static constexpr GLuint INVALID_ID = 0;

private:
	GLuint id_ = INVALID_ID;
};

}
}
}
}

#endif /* RENDERBUFFER_H_ */

