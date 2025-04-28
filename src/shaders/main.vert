#version 450
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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

struct VertexAttributes {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 texCoord;
    vec3 cameraPosition;
    float depth;
};

layout(location = 0) out VertexAttributes outVertexAttributes;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    outVertexAttributes.position = inPosition;
    outVertexAttributes.normal = inNormal;
    outVertexAttributes.color = inColor;
    outVertexAttributes.texCoord = inTexCoord;
    outVertexAttributes.cameraPosition = ubo.cameraPos;
    outVertexAttributes.depth = (ubo.view * vec4(inPosition, 1.0)).z;
}