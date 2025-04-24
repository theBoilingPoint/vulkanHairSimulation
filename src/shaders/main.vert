#version 450
// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots. 
// That means that the index after it must be at least 2 higher:
// layout(location = 0) in dvec3 inPosition;
// layout(location = 2) in vec3 inColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outPositionWorld;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec3 cameraPosition;
layout(location = 5) out float depth;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    outPositionWorld = (ubo.model * vec4(inPosition, 1.0)).xyz;
    outNormal = normalize(inNormal);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    cameraPosition = ubo.cameraPos;
    // TODO: Do I need to flip the sign here?
    depth = (ubo.view * vec4(inPosition, 1.0)).z;
}