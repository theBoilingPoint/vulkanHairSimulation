#include "main.h"
#include "utils.h"

int main() {
	try {
		std::string vertShaderCode = readShaderFile("shaders/main.vert");
		std::vector<uint32_t> vertSPIRV = compileShaderToSPV(vertShaderCode, shaderc_glsl_vertex_shader);
		writeSPVToFile(vertSPIRV, "shaders/vert.spv");
		std::cout << "Vertex shader compiled successfully.\n";

		std::string fragShaderCode = readShaderFile("shaders/main.frag");
		std::vector<uint32_t> fragSPIRV = compileShaderToSPV(fragShaderCode, shaderc_glsl_fragment_shader);
		writeSPVToFile(fragSPIRV, "shaders/frag.spv");
		std::cout << "Fragment shader compiled successfully.\n";

	}
	catch (const std::exception& e) {
		throw std::runtime_error(e.what());
	}

	std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/frag.spv");
	auto [vertices, indices] = loadModel("models/objs/vikingRoom/viking_room.obj");
	const Image image = loadImage("models/objs/vikingRoom/viking_room.png");

	Main app(vertShaderCode, fragShaderCode, vertices, indices, image.width, image.height, image.pixels);

	stbi_image_free(image.pixels);

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
