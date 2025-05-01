#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

#include "bindings.inc"
#include <vector>

struct Vertex {
    glm::vec4 pos;
	glm::vec4 normal;
    glm::vec2 texCoord;
    glm::vec4 color;
    
    bool operator==(const Vertex& other) const {
        return pos == other.pos && 
			normal == other.normal &&
            color == other.color && 
            texCoord == other.texCoord;
    }

    static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = SET_GLOBAL;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return {
			bindingDescription
        };
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		attributeDescriptions.push_back({ BIND_VERTEX_POSITION, SET_GLOBAL, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, pos) });
		attributeDescriptions.push_back({ BIND_VERTEX_NORMAL, SET_GLOBAL, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, normal) });
        attributeDescriptions.push_back({ BIND_VERTEX_TEXCOORD, SET_GLOBAL, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord) });
		attributeDescriptions.push_back({ BIND_VERTEX_COLOR, SET_GLOBAL, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) });

        return attributeDescriptions;
    }
};

// A hash function for Vertex is implemented by specifying a template specialization for std::hash<T>. 
// Hash functions are a complex topic, but https://en.cppreference.com/w/cpp/utility/hash recommends 
// the following approach combining the fields of a struct to create a decent quality hash function:
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}