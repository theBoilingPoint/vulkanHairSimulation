#pragma once

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class Main {
public:
	void run();

private:
	GLFWwindow* window;

	void initVulkan();

	void mainLoop();

	void cleanuUp();
};