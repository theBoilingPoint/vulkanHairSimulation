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
	std::string vertShaderPath = compileShader("shaders/main.vert", "mainVert", Shader::VERTEX);
	std::string triShaderPath = compileShader("shaders/triangle.vert", "triVert", Shader::VERTEX);
	std::string opaqueFragShaderPath = compileShader("shaders/main.frag", "mainFrag", Shader::FRAGMENT);
	std::string weightedColorShaderPath = compileShader("shaders/weightedColor.frag", "weightedColor", Shader::FRAGMENT);
	std::string weightedRevealShaderPath = compileShader("shaders/weightedReveal.frag", "weightedReveal", Shader::FRAGMENT);

	std::unordered_map<std::string, std::vector<char>> shaders = {
		{"vertShader", readFile(vertShaderPath)},
		{"triangleShader", readFile(triShaderPath)},
		{"opaqueFragShader", readFile(opaqueFragShaderPath)},
		{"weightedColorFragShader", readFile(weightedColorShaderPath)},
		{"weightedRevealFragShader", readFile(weightedRevealShaderPath)}
	};

	std::unordered_map<std::string, std::pair<std::vector<Vertex>, std::vector<uint32_t>>> models = {
		{"head", loadModel("assets/models/obj/ponytail/character.obj")},
		{"hair", loadModel("assets/models/obj/ponytail/hair.obj")}
	};
	std::unordered_map<std::string, Image> textures = {
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HEAD_ALBEDO), loadImage("assets/textures/ponytail/Head BaseColor.png")},

		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_ALBEDO), loadImage("assets/textures/ponytail/T_Hair_Basecolor.png")},
		//{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_ALBEDO), loadImage("assets/textures/ponytail/T_Hair_Random Color.png")},
		
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_NORMAL), loadImage("assets/textures/ponytail/T_Hair_Normal.png")},
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_DIRECTION), loadImage("assets/textures/ponytail/T_Hair_Directional.png")},
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_AO), loadImage("assets/textures/ponytail/T_Hair_AO.png")},
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_DEPTH), loadImage("assets/textures/ponytail/T_Hair_Depth.png")},
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_FLOW), loadImage("assets/textures/ponytail/T_Hair_Flow.png")},
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_ROOT), loadImage("assets/textures/ponytail/T_Hair_Root.png")},
		{std::to_string(SET_GLOBAL) + "_" + std::to_string(BIND_HAIR_ID), loadImage("assets/textures/ponytail/T_Hair_ID.png")}
	};

	CubeMap envMap = loadEnvMap("assets/envMaps/christmas_photo_studio_01_4k_hstrip.hdr");

	App app;
	Main vulkanPipeline(
		app.window, 
		app.camera,
		std::move(shaders),
		std::move(models),
		std::move(textures),
		std::move(envMap)
	);

	try {
		vulkanPipeline.mainLoop();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
