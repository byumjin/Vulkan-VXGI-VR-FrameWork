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

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

vec4 uScale = vec4(0.6);
vec4 uBias = vec4(-1.2);

int level = 0;

void main() {

	//extract HDR color
	outColor = max(vec4(0.0), min(textureLod(sceneMap, fragUV, level) + uBias, vec4(10.0))) * uScale;	
	
}