#version 450
#extension GL_KHR_vulkan_glsl : enable
// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots. 
// That means that the index after it must be at least 2 higher:
// layout(location = 0) in dvec3 inPosition;
// layout(location = 2) in vec3 inColor;
// Binding slots defined in bindings.inc
layout(binding = BIND_UBO) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(location = BIND_VERTEX_POSITION) in vec4 inPosition;
layout(location = BIND_VERTEX_NORMAL) in vec4 inNormal;
layout(location = BIND_VERTEX_TEXCOORD) in vec2 inTexCoord;
layout(location = BIND_VERTEX_COLOR) in vec4 inColor;

struct VertexAttributes {
    vec4 position;
    vec4 normal;
    vec2 texCoord;
    vec4 color;
    vec3 cameraPosition;
    float depth;
};

layout(location = 0) out VertexAttributes outVertexAttributes;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * inPosition;
    outVertexAttributes.position = inPosition;
    outVertexAttributes.normal = inNormal;
    outVertexAttributes.color = inColor;
    outVertexAttributes.texCoord = inTexCoord;
    outVertexAttributes.cameraPosition = ubo.cameraPos;
    outVertexAttributes.depth = (ubo.view * inPosition).z;
}