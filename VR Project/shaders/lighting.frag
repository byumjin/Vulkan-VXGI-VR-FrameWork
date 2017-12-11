#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D basicColorMap;
layout(binding = 1) uniform sampler2D specColorMap;
layout(binding = 2) uniform sampler2D normalColorMap;
layout(binding = 3) uniform sampler2D emissiveMap;

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

layout(set = 0, binding = 6) uniform sampler2D DepthMap;

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

layout(set = 0, binding = 7) uniform  DirectionalLights
{
	DirectionalLight instances[1];
} dls;


layout(set = 0, binding = 8) uniform sampler2D shadowMap;

layout(set = 0, binding = 9) uniform  ShadowUniformBuffer
{
	mat4 viewProjMat;
	mat4 invViewProjMat;
} subo;

layout(set = 0, binding = 10) uniform sampler2D GIMap;

layout(set = 0, binding = 11) uniform lightingOption
{
	uint drawMode;
};


layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

float PI = 3.1415926535897932384626422832795028841971f;

vec2 LightingFunGGX_FV(float dotLH, float roughness)
{
	float alpha = roughness*roughness;

	//F
	float F_a, F_b;
	float dotLH5 = pow(clamp(1.0f - dotLH, 0.0f, 1.0f), 5.0f);
	F_a = 1.0f;
	F_b = dotLH5;

	//V
	float vis;
	float k = alpha * 0.5f;
	float k2 = k*k;
	float invK2 = 1.0f - k2;
	vis = 1.0f/(dotLH*dotLH*invK2 + k2);

	return vec2((F_a - F_b)*vis, F_b*vis);
}

float LightingFuncGGX_D(float dotNH, float roughness)
{
	float alpha = roughness*roughness;
	float alphaSqr = alpha*alpha;
	float denom = dotNH * dotNH * (alphaSqr - 1.0f) + 1.0f;

	return alphaSqr / (PI*denom*denom);
}

vec3 GGX_Spec(vec3 Normal, vec3 HalfVec, float Roughness, vec3 BaseColor, vec3 SpecularColor, vec2 paraFV)
{
	float NoH = clamp(dot(Normal, HalfVec), 0.0f, 1.0f);

	float D = LightingFuncGGX_D(NoH * NoH * NoH * NoH, Roughness);
	vec2 FV_helper = paraFV;

	vec3 F0 = SpecularColor;
	vec3 FV = F0*FV_helper.x + vec3(FV_helper.y, FV_helper.y, FV_helper.y);
	
	return D * FV;
}



void main() {

    vec3 resultColor = vec3(0.0);

    vec4 BasicColorMap = texture(basicColorMap, fragUV);
	vec4 SpecColorMap = texture(specColorMap, fragUV);
    vec4 NormalMap = texture(normalColorMap, fragUV);
	vec4 EmissiveMap = texture(emissiveMap, fragUV);
	vec4 GlobalIllumination = texture(GIMap, fragUV);

	float Roughness = SpecColorMap.w;
	float energyConservation = 1.0f - Roughness;
	float Metallic = NormalMap.w;

	//getPosition form Depth
	float depth = texture(DepthMap, fragUV).x;
	
	//get WorldPosition
	vec4 worldPos = ubo.InvViewProjMat * vec4(fragUV.xy * 2.0 - 1.0, depth, 1.0);
	worldPos /= worldPos.w;

	bool bOcculed = false;

	//Ambient
	float AmbientIntensity = 0.05;
	//vec3 AmbientColor = vec3(0.929412, 0.862745, 0.750980);

	resultColor += BasicColorMap.xyz*AmbientIntensity;// * AmbientColor;

	float GIintensity = 1.0;
	GlobalIllumination.xyz *= GIintensity;

	for(int i = 0; i < NumDirectionalLights; i++)
	{

		vec4 shadowWorldPos = subo.viewProjMat * worldPos;
		shadowWorldPos /= shadowWorldPos.w;

		shadowWorldPos.xy = (shadowWorldPos.xy + vec2(1.0))*0.5;

		if((shadowWorldPos.x > 0.0 && shadowWorldPos.x < 1.0) && (shadowWorldPos.y > 0.0 && shadowWorldPos.y < 1.0))
		{
			if(shadowWorldPos.z > texture(shadowMap, shadowWorldPos.xy).x + 0.0001)
			{
				bOcculed = true;
			}
		}
		else
			bOcculed = true;

		if(!bOcculed)
		{
			vec3 NormalVec = NormalMap.xyz;
			vec3 lightDir = -dls.instances[i].lightDirection.xyz;
			vec3 LightVec = normalize(lightDir);
			float NoL = dot(LightVec, NormalVec);

			//Physically-based shader			
			if (NoL > 0.0f)
			{				
				vec3 ViewVec = normalize(ubo.cameraWorldPos - fragWorldPos); 
				vec3 HalfVec = normalize(ViewVec + LightVec);				

				vec3 ReflectVec = -reflect(ViewVec, NormalVec);

				vec3 diffuseTerm = vec3(0.0f);
				vec3 specularTerm = vec3(0.0f);
		
				float LoH = clamp(dot(LightVec, HalfVec), 0.0f, 1.0f);	

				//DIFFUSE
				diffuseTerm = vec3(BasicColorMap.xyz);
				
				//VXGI can't handle metallic reflection because of its distance based contribution
				//diffuseTerm = mix(diffuseTerm, diffuseTerm * GlobalIllumination.xyz / GIintensity, Metallic);

				//SPECULAR
				specularTerm = GGX_Spec(NormalVec, HalfVec, Roughness, diffuseTerm, vec3(SpecColorMap.xyz), LightingFunGGX_FV(LoH, Roughness)) *energyConservation;

				resultColor += (diffuseTerm + specularTerm) * NoL * dls.instances[i].lightInfo.lightColor.xyz * dls.instances[i].lightInfo.lightColor.a;
			}		
		}	
	}

	//drawMode == 0 //Only Lighting
	
	if(drawMode == 1) //Only AO
	{
		 resultColor = vec3(GlobalIllumination.w);
	}
	else if(drawMode == 2) //Lighting + GI
	{
		resultColor += GlobalIllumination.xyz * energyConservation * energyConservation;
	}
	else if(drawMode == 3) //Lighting + AO
	{
		resultColor *= GlobalIllumination.w;
	}
	else if(drawMode == 4) //Lighting + GI + AO
	{
		resultColor += GlobalIllumination.xyz * energyConservation * energyConservation;
		resultColor *= GlobalIllumination.w; // AO
	}

	resultColor += vec3(EmissiveMap.xyz);	

	outColor = vec4(resultColor, 1.0);
}