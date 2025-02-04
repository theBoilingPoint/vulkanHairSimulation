#pragma once

#include <fstream>
#include <shaderc/shaderc.hpp>

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