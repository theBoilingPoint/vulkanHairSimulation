#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(input_attachment_index = 0, set = SET_GLOBAL, binding = BIND_WBOIT_COLOR) uniform subpassInputMS texColor;
layout(input_attachment_index = 1, set = SET_GLOBAL, binding = BIND_WBOIT_REVEAL) uniform subpassInputMS texWeights;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4  accum  = subpassLoad(texColor, gl_SampleID);
	float reveal = subpassLoad(texWeights, gl_SampleID).r;

	// GL blend function: GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA
	outColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);  
}