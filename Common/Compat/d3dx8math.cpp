#include <windows.h>
#include "d3dx8core.h"

#include "d3dx8math.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


static void ConvertGLMToD3DX (const glm::mat4x4 &glm, D3DXMATRIX &d3dx)
{
	d3dx._11 = glm[0][0];
	d3dx._12 = glm[1][0];
	d3dx._13 = glm[2][0];
	d3dx._14 = glm[3][0];

	d3dx._21 = glm[0][1];
	d3dx._22 = glm[1][1];
	d3dx._23 = glm[2][1];
	d3dx._24 = glm[3][1];

	d3dx._31 = glm[0][2];
	d3dx._32 = glm[1][2];
	d3dx._33 = glm[2][2];
	d3dx._34 = glm[3][2];

	d3dx._41 = glm[0][3];
	d3dx._42 = glm[1][3];
	d3dx._43 = glm[2][3];
	d3dx._44 = glm[3][3];
}

static void ConvertD3DXToGLM (const D3DXMATRIX &d3dx, glm::mat4x4 &glm)
{
	glm[0][0] = d3dx._11;
	glm[1][0] = d3dx._12;
	glm[2][0] = d3dx._13;
	glm[3][0] = d3dx._14;

	glm[0][1] = d3dx._21;
	glm[1][1] = d3dx._22;
	glm[2][1] = d3dx._23;
	glm[3][1] = d3dx._24;

	glm[0][2] = d3dx._31;
	glm[1][2] = d3dx._32;
	glm[2][2] = d3dx._33;
	glm[3][2] = d3dx._34;

	glm[0][3] = d3dx._41;
	glm[1][3] = d3dx._42;
	glm[2][3] = d3dx._43;
	glm[3][3] = d3dx._44;
}

D3DXMATRIX::D3DXMATRIX(float m00, float m01, float m02, float m03,
			 float m10, float m11, float m12, float m13,
			 float m20, float m21, float m22, float m23,
			 float m30, float m31, float m32, float m33)
{
	_11 = m00; _12 = m01; _13 = m02; _14 = m03;
	_21 = m10; _22 = m11; _23 = m12; _24 = m13;
	_31 = m20; _32 = m21; _33 = m22; _34 = m23;
	_41 = m30; _42 = m31; _43 = m32; _44 = m33;
}

D3DXMATRIX *WINAPI D3DXMatrixInverse(D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM)
{
	glm::mat4x4 m;
	ConvertD3DXToGLM(*pM, m);

	if (pDeterminant)
		*pDeterminant = glm::determinant(m);

	glm::mat4x4 inv = glm::inverse(m);
	ConvertGLMToD3DX(inv, *pOut);

	return pOut;
}

D3DXMATRIX *WINAPI D3DXMatrixScaling(D3DXMATRIX *pOut, FLOAT sx, FLOAT sy, FLOAT sz)
{
	glm::mat4x4 m = glm::scale(glm::mat4x4(1.0f), glm::vec3(sx, sy, sz));
	ConvertGLMToD3DX(m, *pOut);
	return pOut;
}

D3DXMATRIX *WINAPI D3DXMatrixTranslation(D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z)
{
	glm::mat4x4 m = glm::translate(glm::mat4x4(1.0f), glm::vec3(x, y, z));
	ConvertGLMToD3DX(m, *pOut);
	return pOut;
}

D3DXMATRIX *WINAPI D3DXMatrixMultiply(D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2)
{
	glm::mat4x4 m1, m2;
	ConvertD3DXToGLM(*pM1, m1);
	ConvertD3DXToGLM(*pM2, m2);

	glm::mat4x4 m = m1 * m2;
	ConvertGLMToD3DX(m, *pOut);
	return pOut;
}

D3DXVECTOR4 *WINAPI D3DXVec3Transform(D3DXVECTOR4 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM)
{
	glm::vec4 v(pV->x, pV->y, pV->z, 1.0f);
	glm::mat4x4 m;
	ConvertD3DXToGLM(*pM, m);

	glm::vec4 result = m * v;
	pOut->x = result.x;
	pOut->y = result.y;
	pOut->z = result.z;
	pOut->w = result.w;
	return pOut;
}

D3DXMATRIX *WINAPI D3DXMatrixTranspose(D3DXMATRIX *pOut, CONST D3DXMATRIX *pM)
{
	glm::mat4x4 m;
	ConvertD3DXToGLM(*pM, m);

	glm::mat4x4 mTransposed;
	mTransposed = glm::transpose(m);

	ConvertGLMToD3DX(mTransposed, *pOut);
	return pOut;
}

D3DXMATRIX *WINAPI D3DXMatrixRotationZ(D3DXMATRIX *pOut, FLOAT Angle)
{
	glm::mat4x4 m = glm::rotate(glm::mat4x4(1.0f), Angle, glm::vec3(0.0f, 0.0f, 1.0f));
	ConvertGLMToD3DX(m, *pOut);
	return pOut;
}

D3DXVECTOR4 *WINAPI D3DXVec4Transform(D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, CONST D3DXMATRIX *pM)
{
	glm::vec4 v(pV->x, pV->y, pV->z, pV->w);
	glm::mat4x4 m;
	ConvertD3DXToGLM(*pM, m);

	glm::vec4 result = m * v;
	pOut->x = result.x;
	pOut->y = result.y;
	pOut->z = result.z;
	pOut->w = result.w;
	return pOut;
}

FLOAT WINAPI D3DXVec4Dot(CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2)
{
	glm::vec4 v1(pV1->x, pV1->y, pV1->z, pV1->w);
	glm::vec4 v2(pV2->x, pV2->y, pV2->z, pV2->w);
	return glm::dot(v1, v2);
}