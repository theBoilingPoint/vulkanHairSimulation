#version 450

layout(binding = 1) uniform sampler2D albedoTexSampler;

layout(location = 0) in vec3 inPositionWorld;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in vec3 cameraPosition;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846f;
const float ONE_OVER_PI = 0.31830988618379067154f;

// Point lights
const vec3 light_pos[4] = vec3[](vec3(-10, 10, 10),
                                 vec3(10, 10, 10),
                                 vec3(-10, -10, 10),
                                 vec3(10, -10, 10));

const vec3 light_col[4] = vec3[](vec3(300.f, 300.f, 300.f),
                                 vec3(300.f, 300.f, 300.f),
                                 vec3(300.f, 300.f, 300.f),
                                 vec3(300.f, 300.f, 300.f));

// Fresnel Schlick approximation.
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Smith's method.
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float denom = NdotV * (1.0 - k) + k;
	
    return NdotV / denom;
}
  
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

// Trowbridge-Reitz GGX.
float distributionGGX(vec3 N, vec3 H, float a)
{
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
	
    float denom  = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom        = PI * denom * denom;
	
    return a2 / denom;
}

void main() {
    vec4 albedo = texture(albedoTexSampler, fragTexCoord);
    float metallic = 1.0;
    float roughness = 0.25;
    float ao = 1.0;
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo.xyz, metallic);

    vec3 wo = normalize(cameraPosition - inPositionWorld);
    vec3 diffuse = albedo.xyz * ONE_OVER_PI;
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < light_pos.length(); i++) {
        vec3 wi = normalize(light_pos[i] - inPositionWorld);
        vec3 halfVector = normalize(wi + wo);
        float distance = length(light_pos[i] - inPositionWorld);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light_col[i] * attenuation;
        vec3 F = fresnelSchlick(max(dot(halfVector, wo), 0.0), F0);
        float D = distributionGGX(inNormal, halfVector, roughness);
        float G = geometrySmith(inNormal, wo, wi, roughness);
        vec3 numerator    = D * G * F;
        float denominator = 4.0 * max(dot(inNormal, wo), 0.0) * max(dot(inNormal, wi), 0.0)  + 0.0001;
        vec3 specular     = numerator / denominator; 

        vec3 kd = vec3(1.0) - F;
        kd *= 1.0 - metallic;

        float NdotL = max(dot(inNormal, wi), 0.0);
        Lo += (kd * diffuse + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo.xyz * ao;
    vec3 color   = ambient + Lo;  
    // Reinhard operator: c' = c / (c + 1)
    color = color / (color + vec3(1.0));
    // Gamma correction: c' = c^(1/gamma)
    color = pow(color, vec3(1.0/2.2)); 
    outColor = vec4(color, albedo.w);
}