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

// Parallax mapping 
const float heightScale = 0.005; 
const float minLayers = 10.0;
const float maxLayers = 32.0;

// Noise parameters
const float noiseScale   = 60.0;   // spatial frequency of the sparkles
const float cutoffLow    = 0.65;   // controls how many pixels sparkle
const float cutoffHigh   = 0.95;   // controls sparkle fall‑off

// Hair shading
const float primaryShift = 0.3f; // approx 0.1 – 0.2
const float secondaryShift = -0.3f; // approx −0.1 – −0.2
const float specExp1 = 20;         // approx 20–40
const float specExp2 = 60;         // approx 60–80

// WBOIT parameter
const float distanceWeightExp = -10.0f;

// Point lights
const vec3 light_pos[4] = vec3[](vec3(-10, 10, 10),
                                 vec3(10, 10, 10),
                                 vec3(-10, -10, 10),
                                 vec3(10, -10, 10));

const vec3 light_col[4] = vec3[](vec3(300.f, 300.f, 300.f),
                                vec3(300.f, 300.f, 300.f),
                                vec3(300.f, 300.f, 300.f),
                                vec3(300.f, 300.f, 300.f));

// -----------------------------------------------------------------------------
// Tiny hash & value–noise -----------------------------------------------------
// -----------------------------------------------------------------------------
float hash12(vec2 p)            // 2‑D → 1‑D hash
{
    // large, odd constants minimise visible patterns
    const vec2 k = vec2(127.1, 311.7);
    return fract(sin(dot(p, k)) * 43758.5453123);
}

/* Classic 2‑D value noise (one octave). */
float valueNoise(vec2 p)
{
    vec2  i = floor(p);               // integer lattice cell
    vec2  f = fract(p);               // local position in cell  [0,1)

    // noise at the four corners
    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    // smooth interpolation (quintic curve)
    vec2  u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

/* Four‑octave fBm for richer structure.  
 * ‘scale’ controls frequency; larger = finer sparkle. */
float fbmNoise(vec2 p, float scale)
{
    p *= scale;
    float v   = 0.0;
    float amp = 0.5;

    for (int i = 0; i < 4; ++i)        // 4 octaves is usually enough
    {
        v   += valueNoise(p) * amp;
        p   *= 2.0;                    // next octave
        amp *= 0.5;
    }
    return v;
}


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

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(texDepth, currentTexCoords).r;
  
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(texDepth, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(texDepth, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords; 
} 

vec3 shiftTangent(vec3 tangent, vec3 normal,float shift) {
    vec3 shiftedTangent = tangent + shift * normal;
    return normalize(shiftedTangent);
}

float strandSpecular(vec3 tangent, vec3 viewDir, vec3 lightDir, float exponent) {
    vec3 halfVector = normalize(viewDir + lightDir);
    float dotTH = dot(tangent, halfVector);
    float sinTH = sqrt(1.0 - dotTH * dotTH);
    float disAtten = smoothstep(-1.0, 0.0, dotTH);
    return disAtten * pow(sinTH, exponent);
}

// -----------------------------------------------------------------------------
// Luminance helper ------------------------------------------------------------
// -----------------------------------------------------------------------------
float luminance(vec3 c)                  // ITU‑R BT.709 luma coefficients
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// -----------------------------------------------------------------------------
// Primary & secondary specular tints ------------------------------------------
// -----------------------------------------------------------------------------
/* PRIMARY highlight     – “silk” sheen, subtle, tinted like the fibre itself
 * SECONDARY highlight   – narrower, brighter glints, shifts further to white
 *
 * Both functions use exactly the same logic, differing only in how strongly
 * they drift the base colour towards white.  The drift factor is *adaptive*:
 *   • dark hair (low luminance)        → bigger shift, or you’d never see it
 *   • light hair (high luminance)      → smaller shift, or it would look flat
 */
vec3 computeSpecColor1(vec3 baseCol)     // primary lobe
{
    float Y   = luminance(baseCol);                 // 0 = black, 1 = white
    float amt = mix(0.55, 0.25, Y);                 // dark  → 0.55,   light → 0.25
    return mix(baseCol, vec3(1.0), amt);            // tint towards white
}

vec3 computeSpecColor2(vec3 baseCol)     // secondary lobe
{
    float Y   = luminance(baseCol);
    float amt = mix(0.85, 0.45, Y);                 // stronger drift
    return mix(baseCol, vec3(1.0), amt);
}

vec3 shadeHair(vec3 tangent, vec3 normal, vec2 texCoords, vec3 wo, vec3 wi, vec3 baseCol, vec3 lightCol, float shift, float ao) {
    // -- 1.  Shifted tangents -------------------------------------------------
    vec3 t1 = shiftTangent(tangent, normal, primaryShift + shift);
    vec3 t2 = shiftTangent(tangent, normal, secondaryShift + shift);

    // -- 2.  Diffuse term -----------------------------------------------------
    float NdotL  = dot(normal, wi);
    float diffKW = mix(0.25, 1.0, clamp(NdotL, 0.0, 1.0)); // soft rim
    // vec3  diff   = diffKW * uDiffuseColor;
    vec3  diff = diffKW * baseCol;

    // -- 3.  Specular lobes ---------------------------------------------------
    vec3  spec   = computeSpecColor1(baseCol) * strandSpecular(t1, wo, wi, specExp1);
    const float rawNoise = fbmNoise(texCoords, noiseScale); // approx sparkle map
    const float specMask = smoothstep(cutoffLow, cutoffHigh, rawNoise);          
    float spec2 = strandSpecular(t2, wo, wi, specExp2);
    spec += computeSpecColor2(baseCol) * specMask * spec2;

    // -- 4.  Final colour -----------------------------------------------------
    vec3 result = (diff + spec) * baseCol * lightCol;
    result *= ao; // ambient occl.

    return result;        
}

void main() {
    // Read from textures
    vec3 wo = normalize(inVertexAttributes.cameraPosition - inVertexAttributes.position.xyz);

    float depth = texture(texDepth, inVertexAttributes.texCoord).r;
    vec2 texCoords = parallaxMapping(inVertexAttributes.texCoord, wo);
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) {
        discard;
    }

    vec4 albedo = texture(texAlbedo, texCoords);
    // Alpha cutoff
	// if (albedo.a < 0.01) {
	// 	discard;
	// }

    vec3 normal = texture(texNormal, texCoords).xyz;
	normal = normalize(normal * 2.0 - 1.0);
	normal = normalize(computeTBN(normal) * normal);
    vec2 dirXY = texture(texDirection  , texCoords).rb * 2.0 - 1.0;
    vec3 tangent = normalize(vec3(dirXY, sqrt(max(1.0 - dot(dirXY,dirXY), 0.0))));
	float ao = texture(texAo, texCoords).r;	
	float root = texture(texRoot, texCoords).r;

    // Shade  
    vec3 hairCol = vec3(0.0f);
    for (int i = 0; i < light_pos.length(); i++) {
        hairCol += shadeHair(tangent, normal, texCoords, wo, normalize(light_pos[i] - inVertexAttributes.position.xyz), albedo.rgb, light_col[i] / 100.0f, root, ao);
    }
    hairCol /= float(light_pos.length());

	outColor = vec4(hairCol, albedo.a);
}