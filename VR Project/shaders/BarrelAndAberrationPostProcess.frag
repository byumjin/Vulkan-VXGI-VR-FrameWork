#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 0) uniform sampler2D sceneMap;
//
//
//layout(set = 0, binding = 1) uniform UniformBufferObject
//{
//   mat4 modelMat;
//   mat4 viewMat;
//   mat4 projMat;  
//   mat4 viewProjMat;
//   mat4 InvViewProjMat;
//   mat4 modelViewProjMat;
//   mat4 InvTransposeMat;
//
//   vec3 cameraWorldPos;
//
//} ubo;
//
//layout(location = 0) in vec2 fragUV;
//layout(location = 1) in vec3 fragWorldPos;
//
//layout(location = 0) out vec4 outColor;
//
//void main() {
//    outColor = texture(sceneMap, fragUV);
//}



layout(binding = 0) uniform sampler2D sceneMap;
layout(set = 0, binding = 1) uniform UniformBufferObject
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

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

//only thing on chromatic aberration:https://github.com/spite/Wagner/blob/master/fragment-shaders/chromatic-aberration-fs.glsl 
//need to find good technical article explaining the derivation 

//could also try this kernel:
//from: https://www.reddit.com/r/gamedev/comments/20xyn4/heres_a_great_chromatic_aberration_glsl_function/ 
//// A re-weighted Gaussian kernel
//const vec3 chromaticAberrationKernel[9] = vec3[9](
//vec3(0.0000000000000000000, 0.04416589065853191, 0.0922903086524308425), vec3(0.10497808951021347), vec3(0.0922903086524308425, 0.04416589065853191, 0.0000000000000000000),
//vec3(0.0112445223775533675, 0.10497808951021347, 0.1987116566428735725), vec3(0.40342407932501833), vec3(0.1987116566428735725, 0.10497808951021347, 0.0112445223775533675),
//vec3(0.0000000000000000000, 0.04416589065853191, 0.0922903086524308425), vec3(0.10497808951021347), vec3(0.0922903086524308425, 0.04416589065853191, 0.0000000000000000000)
//); 

vec2 barrelDistortion(vec2 coord, float amt) {
	vec2 cc = coord - 0.5;
	float dist = dot(cc, cc);
	return coord + cc * dist * amt;
}

float sat( float t ) {
	return clamp( t, 0.0, 1.0 );
}

float linterp( float t ) {
	return sat( 1.0 - abs( 2.0*t - 1.0 ) );
}

float remap( float t, float a, float b ) {
	return sat( (t - a) / (b - a) );
}

vec4 spectrum_offset( float t ) {
	vec4 ret;
	float lo = step(t,0.5);
	float hi = 1.0-lo;
	float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
	ret = vec4(lo,1.0,hi, 1.) * vec4(1.0-w, w, 1.0-w, 1.);

	return pow( ret, vec4(1.0/2.2) );
}


//equation from: https://www.imgtec.com/blog/speeding-up-gpu-barrel-distortion-correction-in-mobile-vr/ 
const float alpha = 0.3f;
//given a barrel distored ndc position find its undistorted ndc source position
vec2 BrownsDistortionModel(vec2 x) {
	const float denominator = 1.f / (1.f - alpha*length(x));
	return x * denominator;
}

//given an undistored ndc position find its barrel distored ndc position
vec2 InverseBrownsDistortionModel(vec2 x) { 
	const float denominator = 1.f / (1.f + alpha*length(x));
	return x * denominator;
}

//function from: http://github.prideout.net/barrel-distortion
const float BarrelPower = 2.0f;
//given an ndc vec2 in [-1, 1] return a uv in [0,1]
vec2 Distort(vec2 p) {
    float theta  = atan(p.y, p.x);
    float radius = length(p);
    radius = pow(radius, BarrelPower);
    p.x = radius * cos(theta);
    p.y = radius * sin(theta);
    return 0.5 * (p + 1.0);
}

const float max_distort = 2.2;
const int num_iter = 12;
const float reci_num_iter_f = 1.0 / float(num_iter);
vec2 resolution = vec2(1080,600);

//should definitely try:https://www.youtube.com/watch?v=B7qrgrrHry0&feature=youtu.be&t=11m26s 
const float a = 0.24f;
const float b = 0.22f;
const float c = 1.f - (a + b);

vec2 getOculusBarrelUV(vec2 ndc) {
    // Calculate the source location "radius" (distance from the centre of the viewport)
    // ndc - xy position of the current fragment (destination) in NDC space [-1 1]^2
    float destR = length(ndc);
    float srcR = a * pow(destR,4) + b * pow(destR,2) + c * destR;

    // Calculate the source vector (radial)
    vec2 correctedRadial = normalize(ndc) * srcR;

    // Transform the coordinates (from [-1,1]^2 to [0, 1]^2)
    return (correctedRadial*0.5f) + vec2(0.5f);
}

void main() {	

    //first convert fragUV to its ndc xy
	vec2 ndc = fragUV*2.f - vec2(1.f,1.f);

	vec2 barrelUVsource = getOculusBarrelUV(ndc);
	if(barrelUVsource.x < 0.f || barrelUVsource.x > 1.f || 
	   barrelUVsource.y < 0.f || barrelUVsource.y > 1.f)
	{ outColor = vec4(0.f, 0.f, 0.f, 1.f); return; } 

	//get barrel distored uv source for this ndc destination
//	vec2 barrelUVsource = Distort(ndc);
//	if(barrelUVsource.x < 0.f || barrelUVsource.x > 1.f || 
//	   barrelUVsource.y < 0.f || barrelUVsource.y > 1.f)
//	{ outColor = vec4(0.f, 0.f, 0.f, 1.f); return; } 

//  //looks OK
//	//find the barrel distored source ndc xy
//	vec2 barrelNDCsource = BrownsDistortionModel(ndc);
//
//	//dont color the output if not part of the barrel distortion region
//	if(barrelNDCsource.x < -1.f || barrelNDCsource.x > 1.f || 
//	   barrelNDCsource.y < -1.f || barrelNDCsource.y > 1.f)
//	{ outColor = vec4(0.f, 0.f, 0.f, 1.f); return; } 
//
//	//switch back to uv
//	vec2 barrelUVsource = (barrelNDCsource + vec2(1.f,1.f)) * 0.5f;


	vec4 sumcol = vec4(0.0);
	vec4 sumw = vec4(0.0);	
	for ( int i=0; i<num_iter;++i ) {
		float t = float(i) * reci_num_iter_f;
		vec4 w = spectrum_offset( t );
		sumw += w;
//		sumcol += w * texture( sceneMap, barrelDistortion(fragUV, .6 * max_distort*t ) );
		sumcol += w * texture( sceneMap, barrelDistortion(barrelUVsource, .6 * max_distort*t ) );
	}
		
	outColor = sumcol / sumw;
}
