#include <math.h>
#include <stdio.h>

#include <util/algebra.h>

void algebra_matrix_rotation_x(float *m, float theta)
{
	m[0]=1.0f, m[1]=0.0f, m[2]=0.0f, m[3]=0.0f;
	m[4]=0.0f, m[5]=cos(theta), m[6]=-sin(theta), m[7]=0.0f;
	m[8]=0.0f, m[9]=sin(theta), m[10]=cos(theta), m[11]=0.0f;
	m[12]=0.0f, m[13]=0.0f, m[14]=0.0f, m[15]=1.0f;
}


void algebra_matrix_rotation_y(float *m, float theta)
{
	m[0]=cos(theta), m[1]=0.0f, m[2]=sin(theta), m[3]=0.0f;
	m[4]=0.0f, m[5]=1.0f, m[6]=0.0f, m[7]=0.0f;
	m[8]=-sin(theta), m[9]=0.0f, m[10]=cos(theta), m[11]=0.0f;
	m[12]=0.0f, m[13]=0.0f, m[14]=0.0f, m[15]=1.0f;
}

void algebra_matrix_traslation(float *m, float x, float y, float z)
{
	m[0]=1.0f, m[1]=0.0f, m[2]=0.0f, m[3]=x;
	m[4]=0.0f, m[5]=1.0f, m[6]=0.0f, m[7]=y;
	m[8]=0.0f, m[9]=0.0f, m[10]=1.0f, m[11]=z;
	m[12]=0.0f, m[13]=0.0f, m[14]=0.0f, m[15]=1.0f;
}

void algebra_matrix_multiply(float *product, float *a, float *b)
{
	for (int i=0; i<16; i++)
		product[i]=a[i/4*4]*b[i%4]+a[i/4*4+1]*b[i%4+4]+
		a[i/4*4+2]*b[i%4+8]+a[i/4*4+3]*b[i%4+12];
}

// Projection matrices define a way to project a given region of view space
// onto a plane.

void algebra_matrix_ortho(float *m, float left, float right, float bottom,
float top, float near, float far) {
	m[0]=2/(right-left), m[1]=0.0f, m[2]=0.0f, m[3]=-(right+left)/(right-left);
	m[4]=0.0f, m[5]=2/(top-bottom), m[6]=0.0f, m[7]=-(top+bottom)/(top-bottom);
	m[8]=0.0f, m[9]=0.0f, m[10]=2/(far-near), m[11]=-(far+near)/(far-near);
	m[12]=0.0f, m[13]=0.0f, m[14]=0.0f, m[15]=1.0f;
}

void algebra_matrix_persp(float *m, float fov, float ratio, float n, float f) {
	float t = n*tanf(fov/2); //t=h/2
	float r = t*ratio; //r=w/2
	m[0]=n/r, m[1]=0.0f, m[2]=0.0f, m[3]=0.0f;
	m[4]=0.0f, m[5]=n/t, m[6]=0.0f, m[7]=0.0f;
	m[8]=0.0f, m[9]=0.0f, m[10]=-(f+n)/(f-n), m[11]=-2*f*n/(f-n);
	m[12]=0.0f, m[13]=0.0f, m[14]=-1.0f, m[15]=0.0f;
}
