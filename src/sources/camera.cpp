#include "camera.h"

Camera::Camera() {
	fov = glm::radians(45.0f);
	aspect = 1.0f;
	nearClip = 0.1f;
	farClip = 10.0f;
	position = glm::vec3(2.0f, 2.0f, 2.0f);
	target = glm::vec3(0.0f, 0.0f, 0.0f);
	forward = glm::normalize(target - position);
	up = glm::vec3(0.0f, 0.0f, 1.0f);
	right = glm::normalize(glm::cross(forward, up));

	zoomFactor = 1.0f;
	resized = false;
	view = glm::lookAt(position, target, up);
	proj = glm::perspective(fov, aspect, nearClip, farClip);
}

Camera::~Camera() {}

void Camera::updateAspectRatio(const float aspect) {
	this->aspect = aspect;
	proj = glm::ortho(-aspect * zoomFactor, aspect * zoomFactor, -zoomFactor, zoomFactor, nearClip, farClip);
}

void Camera::zoom() {
	proj = glm::ortho(-aspect * zoomFactor, aspect * zoomFactor, -zoomFactor, zoomFactor, nearClip, farClip);	
}

void Camera::pan(const glm::vec2 delta) {
	position += right * delta.x + up * delta.y;
	target += right * delta.x + up * delta.y;

	view = glm::lookAt(position, target, up);
}

void Camera::rotate(const glm::vec2 radians) {
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), radians.x, glm::vec3(0.0f, 0.0f, 1.0f));
	position = glm::vec3(rotationMatrix * glm::vec4(position - target, 1.0f)) + target;
	forward = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(forward, 0.0f)));
	up = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(up, 0.0f)));
	right = glm::normalize(glm::cross(forward, up));
	
	rotationMatrix = glm::rotate(glm::mat4(1.0f), radians.y, right);
	position = glm::vec3(rotationMatrix * glm::vec4(position - target, 1.0f)) + target;
	forward = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(forward, 0.0f)));
	up = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(up, 0.0f)));

	view = glm::lookAt(position, target, up);
}
