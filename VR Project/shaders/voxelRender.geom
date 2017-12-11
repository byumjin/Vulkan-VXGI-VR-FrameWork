#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (points) in;
layout (triangle_strip, max_vertices = 27) out;

layout(set = 0, binding = 0) uniform UniformBufferObject
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

layout(set = 0, binding = 2) uniform VoxelInfo
{
   vec3 centerPos;
   float maxWidth;
   uint voxelSize;
   uint halfVoxelSize;
   float standardObjScale;
};

layout(location = 0) in vec4 geomColor[];
layout(location = 1) in vec4 outHalfdim[];


layout(location = 0) out vec4 fragColor;

void main(void)
{

	if(geomColor[0].w <= 0.0)
		return;

   //generate Vertex
   
   //gl_Position = gl_in[0].gl_Position;
   vec4 originalPos = gl_in[0].gl_Position;
   
   
   fragColor = geomColor[0] / geomColor[0].w;
   

   //float halfDim = maxWidth / float(voxelSize) * 0.5f;

   float halfDim = outHalfdim[0].x;

   vec4 newPosition;

   //z+
	newPosition = vec4( originalPos.x - halfDim, originalPos.y - halfDim, originalPos.z + halfDim, originalPos.w );
    gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();   
	
    newPosition = vec4( originalPos.x + halfDim, originalPos.y - halfDim, originalPos.z + halfDim, originalPos.w );
    gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	
    newPosition = vec4( originalPos.x - halfDim, originalPos.y + halfDim, originalPos.z + halfDim, originalPos.w );
    gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();

	newPosition = vec4( originalPos.x + halfDim, originalPos.y + halfDim, originalPos.z + halfDim, originalPos.w );
    gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	
	
	
   //+X
	//for degenerate purpose
	EmitVertex();
	newPosition = vec4( originalPos.x + halfDim, originalPos.y - halfDim, originalPos.z + halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x + halfDim, originalPos.y + halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x + halfDim, originalPos.y - halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();


	//-Z
	EmitVertex(); //for degenerate purpose

	newPosition = vec4( originalPos.x - halfDim, originalPos.y - halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x + halfDim, originalPos.y + halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x - halfDim, originalPos.y + halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();

	//-X
	EmitVertex(); //for degenerate purpose
	//f_color = vec4( 0.5, 0.5, 0.5, 1 );
	newPosition = vec4( originalPos.x - halfDim, originalPos.y - halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x - halfDim, originalPos.y + halfDim, originalPos.z + halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x - halfDim, originalPos.y - halfDim, originalPos.z + halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();

	//-Y
	EmitVertex();

	newPosition = vec4( originalPos.x - halfDim, originalPos.y - halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x + halfDim, originalPos.y - halfDim, originalPos.z + halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x + halfDim, originalPos.y - halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();

	//+Y
	EmitVertex();
	//f_color = color;
	newPosition = vec4( originalPos.x + halfDim, originalPos.y + halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	EmitVertex();

	newPosition = vec4( originalPos.x - halfDim, originalPos.y + halfDim, originalPos.z - halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x + halfDim, originalPos.y + halfDim, originalPos.z + halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();
	newPosition = vec4( originalPos.x - halfDim, originalPos.y + halfDim, originalPos.z + halfDim, originalPos.w );
	gl_Position = ubo.modelViewProjMat * newPosition;
	EmitVertex();

	EmitVertex();

	EndPrimitive();
	
}