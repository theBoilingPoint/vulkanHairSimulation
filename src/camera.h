#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
	float fov;
	float aspect;
	float nearClip;
	float farClip;
	glm::vec3 position;
	glm::vec3 target;
	glm::vec3 forward;
	glm::vec3 up;
	glm::vec3 right;

public:
	float zoomFactor;
	glm::mat4 view;
	glm::mat4 proj;

	Camera();
	~Camera();

	void updateAspectRatio(float aspect);

	// Orthographic zoom.
	void zoom();

	void pan(const glm::vec2 delta);

	void rotate(const glm::vec2 radians);
};