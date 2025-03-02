#include "app.h"

App::App() {
	camera = new Camera();
	camera->updateAspectRatio((float)WIDTH / (float)HEIGHT);
	initWindow();
}

App::~App() {
	delete camera;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void App::initWindow() {
	glfwInit();
	// Because GLFW was originally designed to create an OpenGL context, we need to tell it to not create an OpenGL context with a subsequent call.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPositionCallback);
}

void App::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->camera->resized = true;
	app->camera->updateAspectRatio((float)width / (float)height);
}

void App::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

	float curZoomFactor = app->camera->zoomFactor;
	if (yoffset > 0.f) {
		curZoomFactor -= 0.03f;
	}
	else {
		curZoomFactor += 0.03f;
	}
	curZoomFactor = glm::clamp(curZoomFactor, 0.01f, 10.0f);
	app->camera->zoomFactor = curZoomFactor;
	app->camera->zoom();
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

	if (!(button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)) {
		return;
	}

	app->buttonPressed = button;

	if (action == GLFW_PRESS) {
		app->isDragging = true;
		glfwGetCursorPos(window, &app->lastMouseX, &app->lastMouseY);
	}
	else if (action == GLFW_RELEASE) {
		app->isDragging = false;
	}

}

void App::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

	if (!app->isDragging) {
		return;
	}

	app->mouseX = xpos;
	app->mouseY = ypos;

	double deltaX = app->mouseX - app->lastMouseX;
	double deltaY = app->mouseY - app->lastMouseY;

	if (app->buttonPressed == GLFW_MOUSE_BUTTON_LEFT) {
		glm::vec2 delta = glm::vec2(-deltaX, deltaY) * 0.001f;
		app->camera->pan(delta);
	}
	else if (app->buttonPressed == GLFW_MOUSE_BUTTON_RIGHT) {
		glm::vec2 radians = glm::radians(glm::vec2(-deltaX, -deltaY) * 0.2f);
		app->camera->rotate(radians);
	}

	app->lastMouseX = app->mouseX;
	app->lastMouseY = app->mouseY;

}

int main() {
	std::string vertShaderCodeRaw = readShaderFile("shaders/main.vert");
	std::vector<uint32_t> vertSPIRV = compileShaderToSPV(vertShaderCodeRaw, shaderc_glsl_vertex_shader);
	writeSPVToFile(vertSPIRV, "shaders/vert.spv");
	std::cout << "Vertex shader compiled successfully.\n";

	std::string fragShaderCodeRaw = readShaderFile("shaders/main.frag");
	std::vector<uint32_t> fragSPIRV = compileShaderToSPV(fragShaderCodeRaw, shaderc_glsl_fragment_shader);
	writeSPVToFile(fragSPIRV, "shaders/frag.spv");
	std::cout << "Fragment shader compiled successfully.\n";

	std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/frag.spv");
	auto [vertices, indices] = loadModel("models/obj/ponytail/processed.obj");
	const Image image = loadImage("textures/ponytail/T_Hair_Random Color.png");

	App app;
	Main vulkanPipeline(app.window, app.camera, vertShaderCode, fragShaderCode, vertices, indices, image.width, image.height, image.pixels);

	stbi_image_free(image.pixels);

	try {
		vulkanPipeline.mainLoop();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
