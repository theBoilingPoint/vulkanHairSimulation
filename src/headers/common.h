#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

typedef unsigned char stbi_uc;

const int MAX_FRAMES_IN_FLIGHT = 2;

enum Shader {
	VERTEX,
	FRAGMENT
};

struct Image {
	int width;
	int	height;
	int	channels;
	stbi_uc* pixels;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 cameraPos;
};