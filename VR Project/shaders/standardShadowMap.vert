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

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexCol;
layout(location = 2) in vec3 vertexTan;
layout(location = 3) in vec3 vertexBitan;
layout(location = 4) in vec3 vertexNor;
layout(location = 5) in vec2 vertexUV;

layout(location = 0) out vec4 fragPos;

out gl_PerVertex
{
    vec4 gl_Position;
};


void main() {
    gl_Position = viewProjMat * modelMat * vec4(vertexPos, 1.0);

    fragPos = gl_Position / gl_Position.w;
}