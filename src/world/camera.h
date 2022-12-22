#include "common.h"
#include "math/math.h"
#include <mutex>

struct RenderCamera
{
	Mat4x4 VP;

	const Transform* trans;
	float far, near, fov, aspect_ratio;
};


class Camera 
{
	friend class World;
public:
	Camera(u32 width,u32 height,const Transform& trans);

	const Transform& GetTransform() { return m_Transform; }

	
	RenderCamera	OnRender();
	void			OnResize(u32 win_width, u32 win_height);

protected:
	std::mutex	m_AccessLock;
	Transform	m_Transform;
	float m_Far,m_Near,m_AspectRatio,m_Fov;
};

// only one main camera in the scene
class MainCamera : public Camera, public Is_Signleton
{
public:
	MainCamera();
	
	void Initialize(u32 win_width, u32 win_height, const Transform& trans);
};
