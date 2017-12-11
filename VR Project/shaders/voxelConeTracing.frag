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


layout(set = 0, binding = 1) uniform sampler2D DepthMap;
layout(set = 0, binding = 2) uniform sampler2D NormalMap;
layout(set = 0, binding = 3) uniform VoxelInfo
{
   vec3 centerPos;
   float maxWidth;
   uint voxelSize;
   uint halfVoxelSize;
   uint maxLevel;
   float standardObjScale;
}; 

layout(set = 0, binding = 4) uniform sampler3D albedo3DImage;
layout(set = 0, binding = 5) uniform sampler3D albedo3DImageLod01;
layout(set = 0, binding = 6) uniform sampler3D albedo3DImageLod02;
layout(set = 0, binding = 7) uniform sampler3D albedo3DImageLod03;
layout(set = 0, binding = 8) uniform sampler3D albedo3DImageLod04;

layout(set = 0, binding = 9) uniform sampler2D shadowMap;

layout(set = 0, binding = 10) uniform  ShadowUniformBuffer
{
	mat4 viewProjMat;
	mat4 invViewProjMat;
} subo;


struct Light
{
	vec4 focusPosition;
	vec4 lightPosition;
	vec4 lightColor; // a is for intensity
};

struct DirectionalLight
{
	Light lightInfo;
	vec4 lightDirection;
	mat4 viewMat;
	mat4 projMat;
};

int NumDirectionalLights = 1;

layout(set = 0, binding = 11) uniform  DirectionalLights
{
	DirectionalLight instances[1];
} dls;


layout(set = 0, binding = 12) uniform sampler2D specularMap;


struct Ray
{	
	vec4 realOrig;
	vec4 orig;
	vec4 dir;
	vec4 invdir;

	uint sign[4];
}; 

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

vec3 applyNormalMap(vec3 geomnor, vec3 normap) {
   
    vec3 up = normalize(vec3(0.001, 1, 0.001));
    vec3 surftan = normalize(cross(geomnor, up));
    vec3 surfbinor = cross(geomnor, surftan);
    return normalize(normap.y * surftan + normap.x * surfbinor + normap.z * geomnor);
  }


vec3 coneDirections[7] =
{ 
vec3(0.0, 0.0, 1.0),
vec3(0.866025, 0.0, 0.5),
vec3(0.43301270189221932338186158537647, 0.75, 0.5),
vec3(-0.43301270189221932338186158537647, 0.75, 0.5),
vec3(-0.866025, 0.0, 0.5),
vec3(-0.43301270189221932338186158537647, -0.75, 0.5),
vec3(0.43301270189221932338186158537647, -0.75, 0.5)
};


float coneWeights[7] =
{ 
0.22, 0.13, 0.13, 0.13, 0.13, 0.13, 0.13
//0.52, 0.08, 0.08, 0.08, 0.08, 0.08, 0.08
};

float AOWeights[7] =
{ 
0.1429, 0.14285, 0.14285, 0.14285, 0.14285, 0.14285, 0.14285
};

float offsetCorrection[7] =
{ 
5.35, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5
//3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0
};

float offsetSteps[5] =
{ 
//3.0, 3.5, 6.0, 9.0, 10.0
0.1, 0.5, 0.25, 0.1, 0.5
};

float diffuseEnergyConservation[5]=
{ 
	//1.0, 0.89, 0.64, 0.49, 0.36
	1.0, 0.64, 0.36, 0.16, 0.04
	//1.0, 1.0, 1.0, 1.0, 1.0
};

float specularEnergyConservation[5]=
{ 
	1.0, 0.64, 0.36, 0.16, 0.04
};

float getSampleDist(float digonal)
{ 
	return digonal * 0.86602540378443864676372317075294; // cos30;
}

float getSpecularSampleDist(float digonal, float angle)
{ 
	return digonal * cos(0.01745329252 *angle);
}

vec3 getVoxelImageCoords(vec3 worldPos)
{
	vec3 localPos = worldPos* 1.0/standardObjScale - centerPos;
	return  localPos / maxWidth + vec3(0.5);
}

vec3 getBaseVoxelImageCoords(vec3 worldPos)
{
	vec3 localPos = ( worldPos ) * 100.0 - centerPos;
	return  localPos / maxWidth + vec3(0.5);
}

//make noise at screenSpace
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

bool isSameVoxel()
{
	return true;
}

ivec3 get3DtextureSection( vec3 baseCoords, float voxelsize )
{
	baseCoords *= voxelsize;
	return ivec3(floor(baseCoords.x), floor(baseCoords.y), floor(baseCoords.z));
}

void main() {

	vec3 lightDir = -dls.instances[0].lightDirection.xyz;
	vec3 LightVec = normalize(lightDir);

	vec4 SpecularInfo = texture(specularMap, fragUV);

	//getPosition form Depth
	float depth = texture(DepthMap, fragUV).x;
	
	//get WorldPosition
	vec4 worldPos = ubo.InvViewProjMat * vec4(fragUV.xy * 2.0 - 1.0, depth, 1.0);
	worldPos /= worldPos.w;
	
	vec4 worldNormal = texture(NormalMap, fragUV);
	
	float randomSeed = rand(fragUV) * 6.28319; //2PI

	mat3 rotMat;
	float cosTheta = cos(randomSeed);
	float sinTheta = sin(randomSeed);

	rotMat[0] = vec3(cosTheta, sinTheta, 0.0);
	rotMat[1] = vec3(-sinTheta, cosTheta, 0.0);
	rotMat[2] = vec3(0.0, 0.0, 1.0);

	vec3 baseCoords = getBaseVoxelImageCoords(worldPos.xyz);	

	float normalizeWorldVoxelSize = (maxWidth / 512.0) * standardObjScale;  //stad obj scale 

	float WorldVoxelSizeExt = normalizeWorldVoxelSize *  1.4142135;
	float raydistance = normalizeWorldVoxelSize;

	vec3 worldconeDirections[7]; 

	for(int i=0; i <7; i ++)
	{
		worldconeDirections[i] = applyNormalMap(worldNormal.xyz,  normalize(rotMat* normalize(coneDirections[i])));
	}


	

	outColor = vec4(0.0);

	float AO = 0.0;

	//each direction (Diffuse & AO)
	for(int j = 0; j < 7; j++)
	{		
		float offset = 0.0;
		for(int i=0; i <5; i++)
		{			
			float digonal = WorldVoxelSizeExt * pow(2.0f, float(i));// float(i + 1);

			offset = digonal * 0.5 + offsetSteps[i];

			raydistance = getSampleDist(digonal);	
			
			vec3 tracedWorldPos = worldPos.xyz + (offset + raydistance) * worldconeDirections[j];

			

			vec3 coords = getVoxelImageCoords(tracedWorldPos);

			vec4 DiffuseGI = vec4(0.0);
			float localAO = 0.0;


			if(i == 0)
			{
				DiffuseGI = texture(albedo3DImage, coords);	
				localAO = 0.95;				
			}
			else if(i == 1)
			{
				DiffuseGI = texture(albedo3DImageLod01, coords);
				localAO = 0.725;
			}
			else if(i == 2)
			{
				DiffuseGI = texture(albedo3DImageLod02, coords);
				localAO = 0.5;
			}
			else if(i == 3)
			{
				DiffuseGI = texture(albedo3DImageLod03, coords);
				localAO = 0.26;
			}
			else if(i == 4)
			{
				DiffuseGI = texture(albedo3DImageLod04, coords);
				localAO = 0.0;
			}
			else
			{
				break;
			}

			if(DiffuseGI.w > 0.0)
			{				
				
				vec4 shadowWorldPos = subo.viewProjMat * vec4(tracedWorldPos, 1.0);
				shadowWorldPos /= shadowWorldPos.w;

				shadowWorldPos.xy = (shadowWorldPos.xy + vec2(1.0))*0.5;

				bool bOcculed = false;

				if((shadowWorldPos.x > 0.0 && shadowWorldPos.x < 1.0) && (shadowWorldPos.y > 0.0 && shadowWorldPos.y < 1.0))
				{
					if(shadowWorldPos.z > texture(shadowMap, shadowWorldPos.xy).x + 0.001)
					{
						bOcculed = true;
					}
				}
				else
					bOcculed = true;

				DiffuseGI = DiffuseGI/DiffuseGI.w * coneWeights[j] * diffuseEnergyConservation[i];// * NoL;

				if(bOcculed)
					outColor += DiffuseGI * 0.1f;	
				else
					outColor += DiffuseGI * 4.0f;	
				
				
				AO += localAO * AOWeights[j];
							
				break;
			}
		}
	} 

	AO = 1.0 - AO;
	outColor.w = AO;
}