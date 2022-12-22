#include "camera.h"
#include "math/math.h"
#include "renderer/renderer.h"

Camera::Camera(u32 width, u32 height, const Transform& trans) :
	m_Transform(trans), m_Far(1000.f), m_Near(0.1f), m_Fov(Math::pi * .5f)
{
	OnResize(width,height);
}

MainCamera::MainCamera()
	:Camera(600,600,Transform())
{

}

void MainCamera::Initialize(u32 win_width, u32 win_height, const Transform& trans)
{
	OnResize(win_width, win_height);
	m_Transform = trans;

	Singleton<EventSystem>::Get().Register<ResizeEvent>(
		[&](const Event& eve)
		{
			const ResizeEvent& re = (const ResizeEvent&)eve;
			OnResize(re.new_width, re.new_height);
		}
	);
}

RenderCamera Camera::OnRender()
{
	m_AccessLock.lock();

	Mat4x4 proj = Math::perspective(m_AspectRatio, m_Fov, m_Near, m_Far);
	Mat4x4 view = Math::inverse(m_Transform.GetMatrix());
	RenderCamera camera{};
	camera.VP = proj * view;

	camera.aspect_ratio = m_AspectRatio;
	camera.fov = m_Fov;
	camera.trans = &m_Transform;
	camera.near = m_Near;
	camera.far = m_Far;

	m_AccessLock.unlock();
	
	return camera;
}

void Camera::OnResize(u32 win_width, u32 win_height)
{
	m_AccessLock.lock();

	m_AspectRatio = (float)win_width / (float)win_height;

	m_AccessLock.unlock();
}

