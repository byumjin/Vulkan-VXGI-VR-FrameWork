#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D sceneMap;


layout(set = 0, binding = 1) uniform UniformBufferObject
{
   mat4 modelMat;
   mat4 viewMat;
   mat4 projMat;  
   mat4 viewProjMat;
   mat4 InvViewProjMat;
   mat4 modelViewProjMat;
   mat4 InvTransposeMat;

   vec3 cameraWorldPos;

} ubo;

layout(binding = 3) uniform sampler2D bloomMap;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

float bloomCalculation(float x, float A, float B, float C, float D, float E, float F)
{
	return clamp((x/(A/x-C/B)-D/E) / pow(x/(A/x-B)-D/F+E*F, 0.001), 0.0, 1.0);
}

vec4 i_Uncharted2Tonemap(vec4 BlurScene, float fBloom_Threshold)
{
	//float3 x = blur(ReShade::BackBuffer, texcoord, 2.0 * fBloom_Radius);
	float A = (0.15) * fBloom_Threshold;
	float B = (0.50) * fBloom_Threshold;
	float C = (0.10) * fBloom_Threshold;
	float D = (0.20) * fBloom_Threshold;
	float E = (0.02) + fBloom_Threshold;
	float F = (0.30) * fBloom_Threshold;

   return vec4(bloomCalculation(BlurScene.x, A, B, C, D, E, F), bloomCalculation(BlurScene.y, A, B, C, D, E, F), bloomCalculation(BlurScene.z, A, B, C, D, E, F), 1.0);

   //return clamp((x/(A/x-C/B)-D/E) / pow(x/(A/x-B)-D/F+E*F, 0.001), vec4(0.0), vec4(1.0));
}

void main()
 {
    outColor = texture(sceneMap, fragUV) + texture(bloomMap, fragUV);

	//outColor = i_Uncharted2Tonemap(texture(bloomMap, fragUV), 1.0);

	
}