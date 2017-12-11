#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D basicColorTexture;
layout(binding = 1) uniform sampler2D specularColorTexture;
layout(binding = 2) uniform sampler2D normalColorTexture;
layout(binding = 3) uniform sampler2D emissiveColorTexture;

layout(set = 0, binding = 5) uniform UniformBufferObject
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

layout(set = 0, binding = 6) uniform VoxelUniformBufferObject
{
  	mat4 mvpX;
	mat4 mvpY;
	mat4 mvpZ;
	int width;
	int height;
	int voxelSize;
	float halfVoxelSize;
};
/*
layout(set = 0, binding = 7) buffer VoxelFragCount {
 	  uint fragCount;  
};

struct FragmentListData
{
	vec4 data;
};

layout(set = 0, binding = 8) buffer VoxelPos {
 	  FragmentListData voxelPos[];  
};

layout(set = 0, binding = 9) buffer VoxelAlbedo {
 	  FragmentListData voxelAlbedo[];  
};
*/
layout(set = 0, binding = 7, rgba16f) uniform image3D albedo3DImage;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragTangent;
layout(location = 2) in vec3 fragBiTangent;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec2 fragUV;

layout(location = 5) flat in uint faxis;

layout(location = 0) out vec4 outColor;

layout(pixel_center_integer) in vec4 gl_FragCoord;

void main() {

    uvec4 temp = uvec4( gl_FragCoord.x, voxelSize - gl_FragCoord.y, min( float(voxelSize) * gl_FragCoord.z, voxelSize - 1), 0 ) ;
	uvec4 texcoord;

	if( faxis == 0 )
	{
	    texcoord.x = temp.z;
		texcoord.y = temp.y;
		texcoord.z = temp.x;
		
	}
	else if( faxis == 1 )
    {	   
	    texcoord.x = voxelSize - 1 - temp.y;
		texcoord.y = temp.z;		
	    texcoord.z = temp.x;
	}
	else
	{
		texcoord.x = voxelSize - 1 - temp.x;
		texcoord.y = temp.y;
		texcoord.z = temp.z;
	}

	vec4 albedoColor = texture(basicColorTexture, fragUV);

	//alpha clip
	if(albedoColor.w < .1)
	{
		//imageStore( albedo3DImage, ivec3(texcoord.xyz), vec4(0.0));
		discard;
	}

	   
	//uint oldIndex = atomicAdd(fragCount, 1);	


	//voxelPos[oldIndex].data = vec4( texcoord.x + 0.5, texcoord.y + 0.5, texcoord.z + 0.5, 1.0);
	//voxelAlbedo[oldIndex].data = albedoColor;


	//make albedo 3D Texture
	vec4 albedo = imageLoad( albedo3DImage, ivec3(texcoord.xyz));

	albedo += vec4(albedoColor.xyz, 1.0);	
	imageStore( albedo3DImage, ivec3(texcoord.xyz), albedo);
	
			

	//float visualize = float(fragIndex.x) / float(voxelSize* voxelSize * voxelSize);	
	//imageStore( voxelTexture, ivec3(texcoord.xyz), vec4( vec3(visualize), 1.0));
	/*
	vec4 outNormal = texture(normalColorTexture, fragUV);

	vec3 tangentNormal = outNormal.xyz;
	tangentNormal = normalize(tangentNormal * 2.0 - vec3(1.0));
	mat3 tbnMat;
	tbnMat[0] = normalize(fragTangent);
	tbnMat[1] = normalize(fragBiTangent);
	tbnMat[2] = normalize(fragNormal);
	
	vec3 localNormal = tbnMat * tangentNormal;	
	*/
	//vec3 worldNormal = normalize( mat3(ubo.InvTransposeMat) * localNormal );	
	//imageStore( voxelTexture, ivec3(texcoord.xyz), vec4( (localNormal + vec3(1.0)) * 0.5, 1.0));


    outColor = vec4(albedoColor.xyz ,1.0);

}