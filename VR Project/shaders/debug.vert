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

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragWorldPos;

out gl_PerVertex
{
    vec4 gl_Position;
};


void main() {

    fragWorldPos = (ubo.modelMat * vec4(vertexPos, 1.0)).xyz;	

    gl_Position = ubo.viewProjMat *  vec4(fragWorldPos, 1.0);	
	fragUV = vertexUV;
	
}