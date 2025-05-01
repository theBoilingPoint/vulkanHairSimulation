#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// Place this above STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION 
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <tuple>
#include <shaderc/shaderc.hpp>

#include "bindings.inc"
#include "vertex.h"
#include "common.h"

// Function to read a shader source file
std::string readShaderFile(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open shader file: " + filePath);
	}
	return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// Function to compile GLSL to SPIR-V
std::vector<uint32_t> compileShaderToSPV(const std::string& shaderCode, shaderc_shader_kind kind) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Enable optimization
	options.SetOptimizationLevel(shaderc_optimization_level_performance);

	// Compile the shader source code
	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
		shaderCode, kind, "shader", options);

	// Check for compilation errors
	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cerr << "Shader compilation failed:\n" << module.GetErrorMessage() << std::endl;
		throw std::runtime_error("Shader compilation error: " + std::string(module.GetErrorMessage()));
	}

	return { module.cbegin(), module.cend() };
}

// Function to write SPIR-V binary to a file
void writeSPVToFile(const std::vector<uint32_t>& spirvCode, const std::string& outputFilePath) {
	std::ofstream outFile(outputFilePath, std::ios::binary);
	if (!outFile.is_open()) {
		throw std::runtime_error("Failed to open output file: " + outputFilePath);
	}
	outFile.write(reinterpret_cast<const char*>(spirvCode.data()), spirvCode.size() * sizeof(uint32_t));
	outFile.close();
}

std::string compileShader(const std::string path, const std::string shaderName, const Shader type) {
	/* Define the output path. */
	// Handles both '/' and '\' (cross-platform)
	size_t lastSlash = path.find_last_of("/\\");
	std::string shaderPath = "";

	if (lastSlash != std::string::npos) {
		// Include the slash
		shaderPath = path.substr(0, lastSlash + 1);
	}

	/* Add binding slots to the shader code before compiling */
	std::string bindings = readShaderFile("headers/bindings.inc");
	std::string body = readShaderFile(path);

	// Find the position after the #version line
	size_t versionEnd = body.find('\n');
	if (versionEnd == std::string::npos) {
		throw std::runtime_error("Shader file missing #version directive.");
	}

	// Insert bindings after the #version line
	std::string fullCode = body.substr(0, versionEnd + 1) + bindings + "\n" + body.substr(versionEnd + 1);
	std::vector<uint32_t> SPIRV;

	// Uncomment the code below to dump the full shader code to a file for debugging.
//#ifdef _DEBUG
//	{
//		static int dumpCount = 0;
//		std::ofstream out("shaderDump_" + std::to_string(dumpCount++) + ".glsl",
//			std::ios::binary);
//		out << fullCode;
//	}
//#endif

	/* Compile the modified shader code. */
	switch (type) {
	case Shader::VERTEX:
		SPIRV = compileShaderToSPV(fullCode, shaderc_glsl_vertex_shader);
		break;
	case Shader::FRAGMENT:
		SPIRV = compileShaderToSPV(fullCode, shaderc_glsl_fragment_shader);
		break;
	default:
		throw std::runtime_error("Shader type not supported.");
		break;
	}

	std::string newShaderPath = shaderPath + shaderName + ".spv";
	writeSPVToFile(SPIRV, newShaderPath);
	std::cout << "Shader " << shaderName << " compiled successfully.\n";

	return newShaderPath;
}

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

Image loadImage(const std::string& imagePath) {
	if (!std::filesystem::exists(imagePath)) {
		throw std::runtime_error("The image doesn't exist in the relative path: " + imagePath);
	}

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	Image image;
	image.width = texWidth;
	image.height = texHeight;
	image.channels = texChannels;
	image.pixels = pixels;

	return image;
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> loadObj(const std::string& modelPath) {
	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(modelPath)) {
		if (!reader.Error().empty()) {
			throw std::runtime_error(reader.Error());
		}
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader warning: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}
			}
			index_offset += fv;
		}
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
				1.0f
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2],
				0.0f
			};

			// The OBJ format assumes a coordinate system where a vertical coordinate of 0 means the bottom of the image, 
			// however we've uploaded our image into Vulkan in a top to bottom orientation where 0 means the top of the image.
			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = glm::vec4(1.0f);

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	return std::pair(std::move(vertices), std::move(indices));
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> loadGltf(const std::string& modelPath) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = false;
	if (modelPath.find(".glb") != std::string::npos) {
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, modelPath);
	}
	else {
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath);
	}

	if (!warn.empty()) {
		std::cout << "Warn: " << warn << std::endl;
	}
	if (!err.empty()) {
		std::cout << "Err: " << err << std::endl;
	}
	if (!ret) {
		throw std::runtime_error("Failed to parse glTF file: " + modelPath);
	}
	std::cout << "Loaded glTF file: " << modelPath << std::endl;

	return std::pair<std::vector<Vertex>, std::vector<uint32_t>>();
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> loadModel(const std::string& modelPath) {
	if (!std::filesystem::exists(modelPath)) {
		throw std::runtime_error("The file doesn't exist in the relative path: " + modelPath);
	}

	if (modelPath.find(".obj") != std::string::npos) {
		return loadObj(modelPath);
	}
	else if (modelPath.find(".gltf") != std::string::npos || modelPath.find(".glb") != std::string::npos) {
		return loadGltf(modelPath);
	}

	return std::pair<std::vector<Vertex>, std::vector<uint32_t>>();
}