#include "math.h"
#include "glm/gtc/matrix_transform.hpp"

float Math::radians(float degree)
{
	return glm::radians(degree);
}

Vector3f Math::normalize(Vector3f vec)
{
	return glm::normalize(vec);
}

Vector3f Math::cross(Vector3f v1, Vector3f v2)
{
	return glm::cross(v1, v2);
}

float Math::dot(Vector3f v1, Vector3f v2)
{
	return glm::dot(v1, v2);
}

Mat4x4 Math::perspective(float fov, float ratio, float near, float far)
{
	Mat4x4 mat = glm::perspective(fov, ratio, near, far);
	return mat;
}

Mat4x4 Math::position(const Vector3f& offset)
{
	return glm::translate(glm::mat4(1.0f), offset);
}

Mat4x4 Math::rotation(float angle,const Vector3f& v)
{
	return glm::rotate(glm::mat4(1.0f), angle, v);
}

Mat4x4 Math::rotation(const Quaternion& quat)
{
	return quat.GetMatrix();
}

Mat4x4 Math::scale(const Vector3f& s)
{
	return glm::scale(glm::mat4(1.0f), s);
}

Mat4x4 Math::transpose(const Mat4x4& m)
{
	return glm::transpose(m);
}

Mat4x4 Math::inverse(const Mat4x4& m)
{
	return glm::inverse(m);
}

void Transform::SetPosition(const Vector3f& position)
{
	this->position = position;
	RecomputeMatrix();
}

void Transform::SetRotation(const Quaternion& quat)
{
	this->quat = quat;
	RecomputeMatrix();
}

void Transform::SetRotation(const Vector3f& euler)
{
	this->quat = Quaternion(euler);
	RecomputeMatrix();
}

void Transform::SetScale(const Vector3f& scale)
{
	this->scale = scale;
	RecomputeMatrix();
}

void Transform::RecomputeMatrix()
{
	this->world = Math::position(position) * quat.GetMatrix() * Math::scale(scale);
}

Vector3i Math::get_face_dir(FaceCode fcode)
{
	Vector3i faces[6] =
	{
		Vector3i(1,0,0),
		Vector3i(0,1,0),
		Vector3i(0,0,1),
		Vector3i(-1,0,0),
		Vector3i(0,-1,0),
		Vector3i(0,0,-1)
	};
	return faces[(int)fcode];
}

Mat4x4 Math::look_at(Vector3f pos, Vector3f front, Vector3f up)
{
	return glm::lookAt(pos, front, up);
}
