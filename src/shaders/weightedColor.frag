#version 450

layout(binding = 2) uniform sampler2D albedoTexSampler;

struct VertexAttributes {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 texCoord;
    vec3 cameraPosition;
    float depth;
};

layout(location = 0) in VertexAttributes inVertexAttributes;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outReveal;

const float alphaMin = 0.2;
const float alphaWidth = 0.3;

vec3 goochLighting(vec3 normal)
{
  // Light direction
  vec3 light = normalize(vec3(-1, 2, 1));
  // cos(theta), remapped into [0,1]
  float warmth = dot(normalize(normal), light) * 0.5 + 0.5;
  // Interpolate between warm and cool colors (alpha here will be ignored)
  return mix(vec3(0, 0.25, 0.75), vec3(1, 1, 1), warmth);
}

// Applies Gooch shading to a surface with color and alpha and returns
// an unpremultiplied RGBA color.
vec4 shading(vec4 albedo, vec3 normal)
{
  vec3 colorRGB = albedo.rgb * goochLighting(normal);

  // Calculate transparency in [alphaMin, alphaMin+alphaWidth]
  float alpha = clamp(alphaMin + albedo.a * alphaWidth, 0, 1);

  return vec4(colorRGB, alpha);
}

void main() {
	vec4 albedo = texture(albedoTexSampler, inVertexAttributes.texCoord);

	if (albedo.a < 0.01) {
		discard;
	}

	// Insert your favorite weighting function here. The color-based factor
	// avoids color pollution from the edges of wispy clouds. The z-based
	// factor gives precedence to nearer surfaces.

	// The depth functions in the paper want a camera-space depth of 0.1 < z < 500,
	// but the scene at the moment uses a range of about 0.01 to 50, so multiply
	// by 10 to get an adjusted depth:

	//vec4 color = shading(albedo, inVertexAttributes.normal);
	//color.rgb *= color.a;  // Premultiply it
	vec3 tmp = albedo.rgb * albedo.a;
	vec4 color = vec4(tmp, albedo.a);

	const float depthZ = inVertexAttributes.depth * 10.0f; // Z coordinate after applying the view matrix (larger = further away)

	const float distWeight = clamp(0.03 / (1e-5 + pow(depthZ / 200, 4.0)), 1e-2, 3e3);

	float alphaWeight = min(1.0, max(max(color.r, color.g), max(color.b, color.a)) * 40.0 + 0.01);
	alphaWeight *= alphaWeight;

	const float weight = alphaWeight * distWeight;

	// GL Blend function: GL_ONE, GL_ONE
	outColor = color * weight;

	// GL blend function: GL_ZERO, GL_ONE_MINUS_SRC_ALPHA
	outReveal = color.a;
}