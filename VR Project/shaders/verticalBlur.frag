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

layout(set = 0, binding = 2) uniform BlurUniformBufferObject
{
	float widthGap;
	float heightGap;
}bubo;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;


float weights[] = {0.19638062, 0.29675293, 0.094421387, 0.010375977, 0.00025939941};

float gaps[] = {0.0, 1.4117647, 3.2941176, 5.1764706, 7.0588235};

int level = 0;

void main() {

	vec4 resultColor = vec4(0.0);

	resultColor += texture(sceneMap, fragUV) * weights[0];

	for(int i=1; i<5; i++)
	{
		resultColor += texture(sceneMap, fragUV + vec2(0.0, gaps[i]) / bubo.heightGap) * weights[i];
		resultColor += texture(sceneMap, fragUV - vec2(0.0, gaps[i]) / bubo.heightGap) * weights[i];
	}

	outColor = resultColor;
	
}