#pragma once
#include "common.h"
#include "math/math.h"
#include <mutex>

struct RenderCamera
{
	Mat4x4 VP;
	Mat4x4 P;
	Vector3f position;
	Vector3f front;
	float far, near, fov, aspect_ratio;
};


class Camera 
{
	friend class World;
public:
	Camera(u32 width,u32 height,Vector3f pos,Vector3f front);
	
	Vector3f		GetPosition();
	Vector3f		GetFront();

	void			SetPosition(Vector3f pos);
	void			SetFront(Vector3f front);

	float			GetFar();
	float			GetNear();
	float			GetFov();
	
	void			SetFar(float far);
	void			SetNear(float near);
	void			SetFov(float fov);

	RenderCamera	OnRender();
	void			OnResize(u32 win_width, u32 win_height);

protected:
	float m_Far,m_Near,m_AspectRatio,m_Fov;

	Vector3f m_Position;
	Vector3f m_Front;
};

// only one main camera in the scene
class MainCamera : public Camera, public Is_Signleton
{
public:
	MainCamera();
	
	void Initialize(u32 win_width, u32 win_height);
};
