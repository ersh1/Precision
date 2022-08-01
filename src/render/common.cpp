#include "render/common.h"
#include "render/d3d_context.h"

#include <glm/gtc/matrix_transform.hpp>

constexpr const float half_pi = 1.57079632679485f;

float Render::GetFOV() noexcept
{
	// The game doesn't tell us about dyanmic changes to FOV in an easy way (that I can see, but I'm also blind)
	// It does store the FOV indirectly in this global - we can just run the equations it uses in reverse.
	//static const auto fac = Offsets::Get<float*>(513786);
	static const auto fac = (float*)RELOCATION_ID(513786, 391777).address();

	const auto x = *fac / 1.30322540;
	const auto y = glm::atan(x);
	const auto fov = y / 0.01745328f / 0.5f;
	return glm::radians(fov);
}

glm::mat4 Perspective(float fov, float aspect, const RE::NiFrustum& frustum) noexcept
{
	const auto range = frustum.fFar / (frustum.fNear - frustum.fFar);
	const auto height = 1.0f / glm::tan(fov * 0.5f);

	glm::mat4 proj;
	proj[0][0] = height;
	proj[0][1] = 0.0f;
	proj[0][2] = 0.0f;
	proj[0][3] = 0.0f;

	proj[1][0] = 0.0f;
	proj[1][1] = height * aspect;
	proj[1][2] = 0.0f;
	proj[1][3] = 0.0f;

	proj[2][0] = 0.0f;
	proj[2][1] = 0.0f;
	proj[2][2] = range * -1.0f;
	proj[2][3] = 1.0f;

	proj[3][0] = 0.0f;
	proj[3][1] = 0.0f;
	proj[3][2] = range * frustum.fNear;
	proj[3][3] = 0.0f;

	// exact match, save for 2,0 2,1 - looks like XMMatrixPerspectiveOffCenterLH with a slightly
	// different frustum or something. whatever, close enough.
	return proj;
}

// Return the forward view vector
glm::vec3 GetViewVector(const glm::vec3& forwardRefer, float pitch, float yaw) noexcept
{
	auto aproxNormal = glm::vec4(forwardRefer.x, forwardRefer.y, forwardRefer.z, 1.0);

	auto m = glm::identity<glm::mat4>();
	m = glm::rotate(m, -pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	aproxNormal = m * aproxNormal;

	m = glm::identity<glm::mat4>();
	m = glm::rotate(m, -yaw, glm::vec3(0.0f, 0.0f, 1.0f));
	aproxNormal = m * aproxNormal;

	return static_cast<glm::vec3>(aproxNormal);
}

glm::mat4 LookAt(const glm::vec3& pos, const glm::vec3& at, const glm::vec3& up) noexcept
{
	const auto forward = glm::normalize(at - pos);
	const auto side = glm::normalize(glm::cross(up, forward));
	const auto u = glm::cross(forward, side);

	const auto negEyePos = pos * -1.0f;
	const auto sDotEye = glm::dot(side, negEyePos);
	const auto uDotEye = glm::dot(u, negEyePos);
	const auto fDotEye = glm::dot(forward, negEyePos);

	glm::mat4 result;
	result[0][0] = side.x * -1.0f;
	result[0][1] = side.y * -1.0f;
	result[0][2] = side.z * -1.0f;
	result[0][3] = sDotEye * -1.0f;

	result[1][0] = u.x;
	result[1][1] = u.y;
	result[1][2] = u.z;
	result[1][3] = uDotEye;

	result[2][0] = forward.x;
	result[2][1] = forward.y;
	result[2][2] = forward.z;
	result[2][3] = fDotEye;

	result[3][0] = 0.0f;
	result[3][1] = 0.0f;
	result[3][2] = 0.0f;
	result[3][3] = 1.0f;

	return glm::transpose(result);
}

glm::mat4 Render::GetProjectionMatrix(const RE::NiFrustum& frustum) noexcept
{
	const auto& viewSize = Render::GetContext().windowSize;
	const float aspect = viewSize.x / viewSize.y;

	auto f = frustum;
	f.fNear *= RenderScale;
	f.fFar *= RenderScale;

	return Perspective(GetFOV(), aspect, f);
}

glm::mat4 Render::BuildViewMatrix(const glm::vec3& position, const glm::vec2& rotation) noexcept
{
	const auto pos = Render::ToRenderScale(position);
	const auto limit = half_pi * 0.99f;
	const auto dir = GetViewVector(
		{ 0.0, 1.0, 0.0 },
		glm::clamp(rotation.x, -limit, limit),
		rotation.y);
	return LookAt(pos, pos + dir, { 0.0f, 0.0f, 1.0f });
}
