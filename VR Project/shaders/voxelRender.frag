#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;



void main() {

	outColor = fragColor;

}