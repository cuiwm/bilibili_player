#version 130

#ifdef GL_ES
#define varying in
#endif
#line 6

#undef lowp
#undef mediump
#undef highp

//brightness,对比度contrast,饱和度saturation 调整参数

uniform mediump float brightness;
uniform mediump float contrast;
uniform mediump float saturation;

mediump vec3 color_tweak(in mediump vec3 yuv)
{
	mediump float newY, newU, newV;

	newY = ((yuv.x-0.5) * contrast + 0.5) * brightness;
	newU = ((yuv.y-0.5) * saturation + 0.5);
	newV = ((yuv.z-0.5) * saturation + 0.5);

	return vec3(newY, newU, newV);
}

mediump vec3 yuv2rgb(in mediump vec3 yuv)
{
	// YUV offset
	// const vec3 offset = vec3(-0.0625, -0.5, -0.5);
	const mediump vec3 offset = vec3(-0.0625, -0.5, -0.5);
	// RGB coefficients
	const mediump vec3 Rcoeff = vec3( 1.164, 0.000,  1.596);
	const mediump vec3 Gcoeff = vec3( 1.164, -0.391, -0.813);
	const mediump vec3 Bcoeff = vec3( 1.164, 2.018,  0.000);

	mediump vec3 rgb;

	yuv = clamp(yuv, 0.0, 1.0);

	yuv += offset;

	rgb.r = dot(yuv, Rcoeff);
	rgb.g = dot(yuv, Gcoeff);
	rgb.b = dot(yuv, Bcoeff);
	return rgb;
}
