#version 450
#extension GL_KHR_vulkan_glsl : enable

// IMG_WEIGHTED_COLOR, IMG_WEIGHTED_REVEAL are the binding locations (i.e. a number)
// Given my definition of the descriptor set, IMG_WEIGHTED_COLOR = 3 and IMG_WEIGHTED_REVEAL = 4
layout(input_attachment_index = 0, set = 0, binding = 3) uniform subpassInputMS texColor;
layout(input_attachment_index = 1, set = 0, binding = 4) uniform subpassInputMS texWeights;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4  accum  = subpassLoad(texColor, gl_SampleID);
  float reveal = subpassLoad(texWeights, gl_SampleID).r;

  // GL blend function: GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA
  outColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}