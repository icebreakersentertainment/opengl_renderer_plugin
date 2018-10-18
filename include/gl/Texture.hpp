#ifndef GL_TEXTURE_H_
#define GL_TEXTURE_H_

#include <ostream>

#include <GL/glew.h>

#include "OpenGl.hpp"
#include "Bindable.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

template <typename T>
class Texture : public Bindable<T>
{
public:
	Texture() = default;

	explicit Texture(const GLuint id) : id_(id)
	{
	}
	
	Texture(const Texture& other) = delete;
	
	Texture(Texture&& other)
	{
		this->id_ = other.id_;
		
		other.id_ = INVALID_ID;
	}
	
	operator GLuint() const
	{
		return id_;
	}
	
	Texture& operator=(const Texture& other) = delete;
	Texture& operator=(Texture&& other) = default;
	
	void destroy()
	{
		if (!valid()) throw std::runtime_error("Cannot destroy texture - texture was not created.");
		
		glDeleteTextures(numTextures_, &id_);
		
		id_ = INVALID_ID;
		numTextures_ = 0;
	}

	GLuint id() const
	{
		return id_;
	}

	GLsizei numTextures() const
	{
		return numTextures_;
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
	~Texture()
	{
		if (valid())
		{
			destroy();
		}
	}
	
	GLuint id_ = INVALID_ID;
	GLsizei numTextures_ = 0;

};

}
}
}
}

#endif /* GL_TEXTURE_H_ */

