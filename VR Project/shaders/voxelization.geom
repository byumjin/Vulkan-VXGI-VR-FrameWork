#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(set = 0, binding = 6) uniform VoxelUniformBufferObject
{
  	mat4 mvpX;
	mat4 mvpY;
	mat4 mvpZ;
	int width;
	int height;
	int voxelSize;
	float halfVoxelSize;

} vubo;

layout(location = 0) in vec3 geomColor[];
layout(location = 1) in vec3 geomTangent[];
layout(location = 2) in vec3 geomBiTangent[];
layout(location = 3) in vec3 geomNormal[];
layout(location = 4) in vec2 geomUV[];

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragTangent;
layout(location = 2) out vec3 fragBiTangent;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec2 fragUV;

layout(location = 5) flat out uint faxis;

vec3 calculation(vec4 prevPos, vec4 currentPos, vec4 nextPos, vec2 hPixel)
{
	vec3 plane[2];

	plane[0] = cross(currentPos.xyz - prevPos.xyz, prevPos.xyz);
	plane[1] = cross(nextPos.xyz - currentPos.xyz, currentPos.xyz);

	plane[0].z -= dot(hPixel.xy, abs(plane[0].xy));
	plane[1].z -= dot(hPixel.xy, abs(plane[1].xy));

	return cross(plane[0], plane[1]);
}

void main(void)
{
	//Find the axis for the maximize the projected area of this triangle
	vec3 faceNormal = abs( normalize( cross( gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz -gl_in[0].gl_Position.xyz) ) );
	
	mat4 proj;

	float maxi = max(faceNormal.x, max(faceNormal.y, faceNormal.z));


	if( maxi == faceNormal.x )
    {
	    proj = vubo.mvpX;	
		faxis = 0;	
	}
	else if( maxi == faceNormal.y )
    {
	     proj = vubo.mvpY;
		 faxis = 1;	
    }
	else
    {
	     proj = vubo.mvpZ;	
		 faxis = 2;
	}   

    vec4 projPosition[3];
	projPosition[0] = proj * gl_in[0].gl_Position;
	projPosition[1] = proj * gl_in[1].gl_Position;
	projPosition[2] = proj * gl_in[2].gl_Position;

		
	
   //generate Vertex
   for(uint i = 0; i < gl_in.length(); i++)
   {
     gl_Position =  projPosition[i];
	 fragColor = projPosition[i].xyz;
	
	 fragTangent = geomTangent[i];	 
	 fragBiTangent = geomBiTangent[i];
	 fragNormal = geomNormal[i];
	 fragUV = geomUV[i];
     EmitVertex();
   }

   EndPrimitive();
}