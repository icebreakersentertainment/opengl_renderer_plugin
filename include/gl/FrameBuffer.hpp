#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <ostream>

#include <GL/glew.h>

#include "OpenGl.hpp"
#include "Bindable.hpp"
#include "RenderBuffer.hpp"

#include "Types.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class FrameBuffer : public Bindable<FrameBuffer>
{
public:
	FrameBuffer(void) = default;

	explicit FrameBuffer(const GLuint id) : id_(id)
	{
	}
	
	FrameBuffer(const FrameBuffer& other) = delete;
	
	FrameBuffer(FrameBuffer&& other)
	{
		this->id_ = other.id_;
		
		other.id_ = INVALID_ID;
	}
	
	~FrameBuffer()
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
	
	FrameBuffer& operator=(const FrameBuffer& other) = delete;
	FrameBuffer& operator=(FrameBuffer&& other) = default;
	
	void generate()
	{
		if (valid()) throw std::runtime_error("Cannot generate frame buffer - frame buffer was already created.");
		
		glGenFramebuffers(1, &id_);
		
		if (id_ == INVALID_ID)
		{
			throw std::runtime_error("Could not create frame buffer.");
		}
	}
	
	void bind()
	{
		if (!valid()) throw std::runtime_error("Cannot bind frame buffer - frame buffer was not created.");
		
		glBindFramebuffer(GL_FRAMEBUFFER, id_);
		
		ASSERT_GL_ERROR();
	}
	
	void attach(const Texture2d& texture)
	{
		if (!valid()) throw std::runtime_error("Cannot attach texture to frame buffer - frame buffer was not created.");
		
		bind();
		
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + numAttachments_, GL_TEXTURE_2D, texture, 0);
		
		numAttachments_++;
		
		// GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, ...
		std::vector<GLuint> attachments;
		for (int i=0; i < numAttachments_; ++i)
		{
			attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		
		glDrawBuffers(numAttachments_, &attachments[0]);
		
		ASSERT_GL_ERROR();
	}
	
	void attach(const Texture2d& texture, const GLenum attachment)
	{
		if (!valid()) throw std::runtime_error("Cannot attach texture to frame buffer - frame buffer was not created.");
		
		bind();
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
		
		ASSERT_GL_ERROR();
	}
	
	void attach(const RenderBuffer& renderBuffer, const GLenum attachment)
	{
		if (!valid()) throw std::runtime_error("Cannot attach render buffer to frame buffer - frame buffer was not created.");
		
		bind();
		
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderBuffer);
		
		ASSERT_GL_ERROR();
	}
	
	GLenum status()
	{
		if (!valid()) throw std::runtime_error("Cannot get status of frame buffer - frame buffer was not created.");
		
		bind();
		
		return glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}
	
	bool ready()
	{
		if (!valid()) throw std::runtime_error("Cannot get status of frame buffer - frame buffer was not created.");
		
		bind();
		
		return status() == GL_FRAMEBUFFER_COMPLETE;
	}
	
	void destroy()
	{
		if (!valid()) throw std::runtime_error("Cannot destroy frame buffer - frame buffer was not created.");
		
		glDeleteFramebuffers(1, &id_);
		
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
	
	static void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		ASSERT_GL_ERROR();
	}
	
	static void drawBuffer(const GLenum buf)
	{
		glDrawBuffer(buf);
		
		ASSERT_GL_ERROR();
	}
	
	static void readBuffer(const GLenum mode)
	{
		glReadBuffer(mode);
		
		ASSERT_GL_ERROR();
	}
	
	static constexpr GLuint INVALID_ID = 0;

private:
	GLuint id_ = INVALID_ID;
	GLsizei numAttachments_ = 0;
};

}
}
}
}

#endif /* FRAMEBUFFER_H_ */

