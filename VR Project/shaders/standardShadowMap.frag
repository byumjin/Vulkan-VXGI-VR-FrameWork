#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform  ShadowUniformBuffer
{
	mat4 viewProjMat;
	mat4 invViewProjMat;
};

layout(set = 0, binding = 1) uniform  ObjectUniformBuffer
{
	mat4 modelMat;
};

layout(location = 0) in vec4 fragPos;

layout(location = 0) out float outDepth;

void main() {
	outDepth = fragPos.z;
}