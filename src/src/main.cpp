#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "main.h"

void Main::run() {
	initVulkan();
	mainLoop();
	cleanuUp();
}

void Main::initVulkan() {
	glfwInit();
	// Because GLFW was originally designed to create an OpenGL context, we need to tell it to not create an OpenGL context with a subsequent call.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void Main::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Main::cleanuUp() {
	glfwDestroyWindow(window);

	glfwTerminate();
}


int main() {
	Main app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}