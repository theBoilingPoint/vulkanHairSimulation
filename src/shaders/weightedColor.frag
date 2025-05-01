#version 450

layout(binding = BIND_HAIR_ALBEDO) uniform sampler2D texAlbedo;
layout(binding = BIND_HAIR_NORMAL) uniform sampler2D texNormal;
layout(binding = BIND_HAIR_DIRECTION) uniform sampler2D texDirection;
layout(binding = BIND_HAIR_AO) uniform sampler2D texAo;
layout(binding = BIND_HAIR_DEPTH) uniform sampler2D texDepth;
layout(binding = BIND_HAIR_ROOT) uniform sampler2D texRoot;
layout(binding = BIND_HAIR_FLOW) uniform sampler2D texFlow;

struct VertexAttributes {
    vec4 position;
    vec4 normal;
    vec2 texCoord;
    vec4 color;
    vec3 cameraPosition;
    float depth;
};

layout(location = 0) in VertexAttributes inVertexAttributes;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outReveal;

const float alphaMin = 0.2;
const float alphaWidth = 0.3;

// Point lights
const vec3 light_pos[4] = vec3[](vec3(-10, 10, 10),
                                 vec3(10, 10, 10),
                                 vec3(-10, -10, 10),
                                 vec3(10, -10, 10));

const vec3 light_col[4] = vec3[](vec3(300.f, 300.f, 300.f),
                                vec3(300.f, 300.f, 300.f),
                                vec3(300.f, 300.f, 300.f),
                                vec3(300.f, 300.f, 300.f));

mat3 computeTBN(in vec3 normal) {
	// Screen-space derivatives of position and UV
    vec3 pos_dx = dFdx(inVertexAttributes.position.xyz);
    vec3 pos_dy = dFdy(inVertexAttributes.position.xyz);
    vec2 uv_dx  = dFdx(inVertexAttributes.texCoord);
    vec2 uv_dy  = dFdy(inVertexAttributes.texCoord);

    float det = uv_dx.x * uv_dy.y - uv_dx.y * uv_dy.x;
    // Small epsilon to avoid divide-by-zero on hair cards whose UVs are almost colinear
    float invDet = 1.0 / max(abs(det), 1e-6);

    vec3 tangent   = ( pos_dx *  uv_dy.y - pos_dy *  uv_dx.y) * invDet;
    vec3 bitangent = (-pos_dx *  uv_dy.x + pos_dy *  uv_dx.x) * invDet;

    // Orthonormalise
    vec3 n  = normalize(normal);
    vec3 t  = normalize(tangent   - n * dot(n, tangent));
    vec3 b  = normalize(bitangent - n * dot(n, bitangent));

    // Optional handedness flip
    if (det < 0.0) {
		b = -b;
	} 

	return mat3(t, b, n);
}

// Kajiya-Kay diffuse + dual-lobe spec (simplified Marschner)
vec3 shadeHair(vec3 N, vec3 T, vec3 V, vec3 L,
               vec3 baseCol, float roughness, float shift)
{
    vec3 B = normalize(cross(T, N));             // bitangent
    // Diffuse term (wrap)
    float NoL = clamp(dot(N, L) * 0.5 + 0.5, 0.0, 1.0);
    vec3  diff = baseCol * NoL;

    // Longitudinal (along-strand) spec
    vec3  H = normalize(L + V);
    float Th = dot(T, H);
    float spec1 = pow(max(Th, 0.0), 1.0 / (roughness + 1e-4));

    // Shifted secondary lobe (makes that ��silky rim��)
    float spec2 = pow(max(dot(T, normalize(L + V + shift*N)), 0.0),
                      1.0 / ((roughness*0.5) + 1e-4));

    return diff + vec3(0.04) * spec1 + vec3(0.04) * spec2;
}

void main() {
    // Read from textures
	vec4 albedo = texture(texAlbedo, inVertexAttributes.texCoord);

	if (albedo.a < 0.01) {
		discard;
	}

	vec3 normal = texture(texNormal, inVertexAttributes.texCoord).xyz;
	normal = normalize(normal * 2.0 - 1.0);
	normal = normalize(computeTBN(normal) * normal);
    vec2 dirXY = texture(texDirection  , inVertexAttributes.texCoord).rb * 2.0 - 1.0;
    vec3 tangent = normalize(vec3(dirXY, sqrt(max(1.0 - dot(dirXY,dirXY), 0.0))));
	float ao = texture(texAo, inVertexAttributes.texCoord).r;
	float depth = texture(texDepth, inVertexAttributes.texCoord).r;
	float root = texture(texRoot, inVertexAttributes.texCoord).r;

    // Compute the directions
    vec3 wo = normalize(inVertexAttributes.cameraPosition - inVertexAttributes.position.xyz);

    // Shade
    float roughness = mix(0.15, 0.4, root); // tighter near root
    vec3 litColor = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        vec3 wi = normalize(light_pos[i] - inVertexAttributes.position.xyz);
        float distance = length(light_pos[i] - inVertexAttributes.position.xyz);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light_col[i] * attenuation;

        litColor += shadeHair(normal, tangent, wo, wi, albedo.rgb, roughness, 0.35f) * radiance;
    }

    litColor *= mix(1.0, 0.6, depth) * ao;      

    // WBOIT output
	// Insert your favorite weighting function here. The color-based factor
	// avoids color pollution from the edges of wispy clouds. The z-based
	// factor gives precedence to nearer surfaces.

	// The depth functions in the paper want a camera-space depth of 0.1 < z < 500,
	// but the scene at the moment uses a range of about 0.01 to 50, so multiply
	// by 10 to get an adjusted depth:
	vec3 tmp = litColor.rgb * albedo.a; // Premultiply it
	vec4 color = vec4(tmp, albedo.a);
	const float depthZ = -inVertexAttributes.depth * 1000.0f; // Z coordinate after applying the view matrix (larger = further away)
	const float distWeight = clamp(0.03 / (1e-5 + pow(depthZ / 200, 4.0)), 1e-2, 3e3);
	float alphaWeight = min(1.0, max(max(color.r, color.g), max(color.b, color.a)) * 40.0 + 0.01);
	alphaWeight *= alphaWeight;
	const float weight = alphaWeight * distWeight;

	// GL Blend function: GL_ONE, GL_ONE
	outColor = color * weight;

	// GL blend function: GL_ZERO, GL_ONE_MINUS_SRC_ALPHA
	outReveal = color.a;
}