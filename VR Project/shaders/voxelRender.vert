#version 450
#extension GL_ARB_separate_shader_objects : enable

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
   uint maxLevel;
   float standardObjScale;
};

struct FragmentListData
{
	vec4 data;
};


layout(set = 0, binding = 3) buffer VoxelPos {
 	  FragmentListData voxelPos[];  
};

layout(set = 0, binding = 4) buffer VoxelAlbedo {
 	  FragmentListData voxelAlbedo[];  
};

struct SVONode
{
	uint index;
	uint parentIndex;	
	uint level;	
	uint fragListIndex;
	uint fragListIndexArray[8];
	uint childrenIndex[8];

	vec4 centerPos;
	vec4 albedo;
	//vec4 padding[7];

	//vec4 childrenPos[8];
};

struct BoundBox
{
	vec4 bounds[2]; //min and max
};

struct OctreeNode
{
	uint index;
	uint parentIndex;	
	uint level; //3
	uint workGroupID[3]; //6
	uint childrenIndex[8]; //16 
	uint padding[2];

	BoundBox bb;
	vec4 albedo;
};

layout(set = 0, binding = 5) buffer OctreeBuffer {
 	  OctreeNode Node[];  
};


layout(set = 0, binding = 6) uniform sampler3D albedo3DImage;

layout(location = 0) out vec4 geomColor;

out gl_PerVertex
{
    vec4 gl_Position;
};


bool isBound(vec3 voxelPos, vec3 centerPos, float halfLevelSize)
{
	if( (voxelPos.x > (centerPos.x - halfLevelSize)) && (voxelPos.x < (centerPos.x + halfLevelSize)) && 
	(voxelPos.y > (centerPos.y - halfLevelSize)) && (voxelPos.y < (centerPos.y + halfLevelSize)) && 
	(voxelPos.z > (centerPos.z - halfLevelSize)) && (voxelPos.z < (centerPos.z + halfLevelSize)) )
	{
		return true;
	}
	else 
		return false;
}

uint passedSubNode(in vec3 voxelPos, in vec3 CenterPos, in float halfLevelSize)
{
	float hh = halfLevelSize * 0.5;

	if(isBound(voxelPos, vec3(CenterPos.x - hh, CenterPos.y - hh, CenterPos.z - hh),  hh))
	{
		return 0;
	}
	else if(isBound(voxelPos, vec3(CenterPos.x + hh, CenterPos.y - hh, CenterPos.z - hh),  hh))
	{
		return 1;
	}
	else if(isBound(voxelPos, vec3(CenterPos.x - hh, CenterPos.y + hh, CenterPos.z - hh),  hh))
	{
		return 2;
	}
	else if(isBound(voxelPos, vec3(CenterPos.x + hh, CenterPos.y + hh, CenterPos.z - hh),  hh))
	{
		return 3;
	}
	else if(isBound(voxelPos, vec3(CenterPos.x - hh, CenterPos.y - hh, CenterPos.z + hh),  hh))
	{
		return 4;
	}
	else if(isBound(voxelPos, vec3(CenterPos.x + hh, CenterPos.y - hh, CenterPos.z + hh),  hh))
	{
		return 5;
	}
	else if(isBound(voxelPos, vec3(CenterPos.x - hh, CenterPos.y + hh, CenterPos.z + hh),  hh))
	{
		return 6;
	}
	else if(isBound(voxelPos, vec3(CenterPos.x + hh, CenterPos.y + hh, CenterPos.z + hh),  hh))
	{	
		return 7;
	}
	else
	{
		return 999;
	}

}

layout(location = 1) out vec4 outHalfdim;

#define USE_OCTREE 0

void main() {
	
	
	geomColor = vec4(0.0);

	uint controlMaxLevel = maxLevel;

#if USE_OCTREE

	
	vec3 v_texcoord = voxelPos[gl_VertexIndex].data.xyz;

	uint currentLevelSize = voxelSize;
	uint halfLevelSize = currentLevelSize/2;

	Node[0].centerPos.xyz = vec3(float(halfLevelSize));
		
	uint currentIndex = 0;	

	

	for(uint k = 0; k <= controlMaxLevel; k++)
	{
		if(k == controlMaxLevel)
		{
			v_texcoord = Node[currentIndex].centerPos.xyz;
			v_texcoord /= float(voxelSize);
			v_texcoord -= vec3(0.5);
			v_texcoord *= maxWidth;
			v_texcoord += centerPos;
	
			gl_Position = vec4( v_texcoord, 1.0 );			
			geomColor =  Node[currentIndex].albedo;


			outHalfdim.x =  maxWidth / (float(voxelSize) * pow(0.5, float(maxLevel -  controlMaxLevel))) * 0.5f;
			

			break;
		}
		else
		{						
			uint result = passedSubNode( v_texcoord, Node[currentIndex].centerPos.xyz, float(halfLevelSize));
			
			if(result < 8)
			{
				currentIndex = Node[currentIndex].childrenIndex[result];
				halfLevelSize /= 2;
			}
			else
			{
				geomColor = vec4(1,0,0,0);
				break;
			}
		}
	}
	
#else
	
	vec3 v_texcoord = voxelPos[gl_VertexIndex].data.xyz;

	

	v_texcoord /= float(voxelSize);

	vec4 COLOR =  texture(albedo3DImage, v_texcoord.xyz );

	if(COLOR.w <= 0.0)
		return;

	geomColor = COLOR;

	


	v_texcoord -= vec3(0.5);
	v_texcoord *= maxWidth;
	v_texcoord += centerPos;
	
	gl_Position = vec4( v_texcoord, 1.0 );
	
	outHalfdim.x =  maxWidth / (float(voxelSize) * pow(0.5, float(maxLevel -  controlMaxLevel))) * 0.5f;

    
	//geomColor = voxelAlbedo[gl_VertexIndex].data;

	
	

#endif

}