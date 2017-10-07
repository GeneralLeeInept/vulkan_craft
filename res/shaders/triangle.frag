#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{	
	float NdotL = clamp(dot(fragNormal, vec3(0.58)), 0, 1);
    outColor = clamp(NdotL + 0.5, 0, 1) * texture(texSampler, fragTexCoord);
}
