#version 150

uniform vec4 color;
uniform float metallic;
uniform float roughness;
uniform vec3 viewPos;

in vec3 normalFrag;
in vec3 fragPos;
in vec3 vertexColor;
out vec4 outColor;

const float PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float alpha)
{
	float a2 = alpha * alpha;
	float NdotH = max(dot(N, H), 0.0);
	float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;
	return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float k)
{
	return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
	float ggx1 = geometrySchlickGGX(max(dot(N, V), 0.0), k);
	float ggx2 = geometrySchlickGGX(max(dot(N, L), 0.0), k);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
	vec3 lightDirections[3];
	vec3 lightColors[3];
	lightDirections[0] = normalize(vec3(1.0, 2.0, 3.0));
	lightDirections[1] = normalize(vec3(-1.0, 2.0, -3.0));
	lightDirections[2] = normalize(vec3(0.0, -1.0, 0.0));
	lightColors[0] = vec3(5.0);
	lightColors[1] = vec3(5.0);
	lightColors[2] = vec3(3.0);

	vec3 N = normalize(normalFrag);
	vec3 V = normalize(viewPos - fragPos);
	vec3 albedo = pow(color.rgb * vertexColor, vec3(2.2));
	float metal = clamp(metallic, 0.0, 1.0);
	float rough = clamp(roughness, 0.04, 1.0);
	float alpha = rough * rough;
	float k = pow(rough + 1.0, 2.0) / 8.0;
	vec3 F0 = mix(vec3(0.04), albedo, metal);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < 3; ++i)
	{
		vec3 L = normalize(lightDirections[i]);
		vec3 H = normalize(V + L);
		float NdotL = max(dot(N, L), 0.0);
		if(NdotL > 0.0)
		{
			float NDF = distributionGGX(N, H, alpha);
			float G = geometrySmith(N, V, L, k);
			vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

			vec3 specular = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * NdotL, 0.001);
			vec3 kS = F;
			vec3 kD = (vec3(1.0) - kS) * (1.0 - metal);

			Lo += (kD * albedo / PI + specular) * lightColors[i] * NdotL;
		}
	}

	vec3 ambient = 0.03 * albedo;
	vec3 radiance = ambient + Lo;
	radiance = pow(radiance, vec3(1.0 / 2.2));
	outColor = vec4(radiance, color.a);
}
