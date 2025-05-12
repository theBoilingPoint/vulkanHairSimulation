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

class Cube {
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
    uint32_t count;

    Cube() {  
        /* 8 corner positions ------------------------------------------------ */
        const std::vector<glm::vec3> pos = {
            {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
            {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
        };

        /* 36 indices (12 triangles, 6 faces) -------------------------------- */
        const std::vector<uint32_t> idx = {
            1, 0, 3, 1, 3, 2,        // –Z
            4, 5, 6, 4, 6, 7,        // +Z
            5, 1, 2, 5, 2, 6,        // +X
            7, 6, 2, 7, 2, 3,        // +Y
            0, 4, 7, 0, 7, 3,        // –X
            0, 1, 5, 0, 5, 4         // –Y
        };

        /* build vertex array ------------------------------------------------- */
        vertices.reserve(pos.size());
        for (const glm::vec3& p : pos)
        {
            Vertex v{};
            v.pos = glm::vec4(p, 1.0f);

            // Per‑vertex normal = normalized position —‑ smooth‑shaded cube.
            // (If you want hard edges: duplicate vertices per face instead.)
            v.normal = glm::vec4(glm::normalize(p), 0.0f);

            // Quick planar UV (x,y mapped to [0,1]).
            v.texCoord = glm::vec2((p.x + 1.0f) * 0.5f,
                (p.y + 1.0f) * 0.5f);

            // White by default; tint if you prefer.
            v.color = glm::vec4(1.0f);

            vertices.push_back(v);
        }

        /* copy index list & store triangle count ----------------------------- */
        indices = idx;
        count = static_cast<uint32_t>(indices.size());
    }

	std::pair<std::vector<Vertex>, std::vector<uint32_t>> getMesh() {
		return { vertices, indices };
	}
};