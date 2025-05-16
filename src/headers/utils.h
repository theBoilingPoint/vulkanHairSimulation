#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <tuple>
#include <shaderc/shaderc.hpp>
#include <array>

#include "bindings.inc"
#include "vertex.h"

/* Constants */
const int MAX_FRAMES_IN_FLIGHT = 2;

/* Types and Structs */
typedef unsigned char stbi_uc;
typedef std::array<float*, 6> CubeMapData;

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

struct HDRImage {
	int width;
	int	height;
	int	channels;
	float* pixels;
};

struct CubeMap {
	// Each face is a square so no need to store width and height separately.
	int resolution;
	int channels;
	CubeMapData faces;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 cameraPos;
};

struct UIState {
	bool transparencyOn;
};

/* Functions */
// Function to read a shader source file
std::string readShaderFile(const std::string& filePath);

// Function to compile GLSL to SPIR-V
std::vector<uint32_t> compileShaderToSPV(const std::string& shaderCode, shaderc_shader_kind kind);

// Function to write SPIR-V binary to a file
void writeSPVToFile(const std::vector<uint32_t>& spirvCode, const std::string& outputFilePath);

std::string compileShader(const std::string path, const std::string shaderName, const Shader type);

std::vector<char> readFile(const std::string& filename);

Image loadImage(const std::string& imagePath);

// 6 × RGBA32F face pointers (or RGB)
CubeMap loadFlattenedEnvMap(const std::string& imagePath);

HDRImage loadEnvMap(const std::string& imagePath);

std::pair<std::vector<Vertex>, std::vector<uint32_t>> loadObj(const std::string& modelPath);

std::pair<std::vector<Vertex>, std::vector<uint32_t>> loadGltf(const std::string& modelPath);

std::pair<std::vector<Vertex>, std::vector<uint32_t>> loadModel(const std::string& modelPath);