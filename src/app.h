#pragma once

#include "camera.h"
#include "main.h"
#include "utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class App {
private:
	bool isDragging = false;
	double lastMouseX, lastMouseY;
	double mouseX, mouseY;
	unsigned int buttonPressed;

	void initWindow();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

public:
	GLFWwindow* window;
	Camera* camera;

	App();
	~App();
};
