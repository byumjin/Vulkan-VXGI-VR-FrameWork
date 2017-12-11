#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 4) uniform UniformBufferObject
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

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexCol;
layout(location = 2) in vec3 vertexTan;
layout(location = 3) in vec3 vertexBitan;
layout(location = 4) in vec3 vertexNor;
layout(location = 5) in vec2 vertexUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragTangent;
layout(location = 2) out vec3 fragBiTangent;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec2 fragUV;

out gl_PerVertex
{
    vec4 gl_Position;
};


void main() {
    gl_Position = vec4(vertexPos, 1.0);

    fragColor = vertexCol;

	fragTangent = normalize(vertexTan);
	fragBiTangent = normalize(vertexBitan);
	fragNormal = normalize(vertexNor);

	fragUV = vertexUV;
}