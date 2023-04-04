#include "camera.h"
#include "math/math.h"

Camera::Camera(u32 width, u32 height, Vector3f pos,Vector3f front) :
	 m_Far(1000.f), m_Near(0.01f), m_Fov(Math::pi * .5f)
{
	OnResize(width,height);
}

Vector3f Camera::GetPosition()
{
	return m_Position;
}

Vector3f Camera::GetFront()
{
	return m_Front;
}

void Camera::SetPosition(Vector3f pos)
{
	m_Position = pos;
}

void Camera::SetFront(Vector3f front)
{
	m_Front = front;
}

float Camera::GetFar()
{
	return m_Far;
}

float Camera::GetNear()
{
	return m_Near;
}

void Camera::SetFar(float far)
{
	m_Far = far;
}

void Camera::SetNear(float near)
{
	m_Near = near;
}

void Camera::SetFov(float fov)
{
	m_Fov = fov;
}

float Camera::GetFov()
{
	return m_Fov;
}

MainCamera::MainCamera()
	:Camera(600,600,Vector3f(),Vector3f(0,0,1))
{

}

void MainCamera::Initialize(u32 win_width, u32 win_height)
{
	OnResize(win_width, win_height);
}

RenderCamera Camera::OnRender()
{
	Mat4x4 proj = Math::perspective(m_Fov, m_AspectRatio, m_Near, m_Far);
	Mat4x4 view = Math::look_at(m_Position, m_Position + m_Front, Vector3f(0, 1, 0));
	proj[0][0] *= -1;
	proj[1][1] *= -1;

	RenderCamera camera{};
	camera.VP = proj * view;
	camera.P = proj;

	camera.aspect_ratio = m_AspectRatio;
	camera.fov = m_Fov;
	camera.near = m_Near;
	camera.far = m_Far;

	camera.position = m_Position;
	camera.front = m_Front;
	
	return camera;
}

void Camera::OnResize(u32 win_width, u32 win_height)
{
	vkmc_assert(win_height != 0);
	m_AspectRatio = (float)win_width / (float)win_height;
}

