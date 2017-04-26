//------------------------------------------------------------------------------
// File: Sky.h
// Desc: Sky mesh with procedural cloud shader
//
// Created: 04 January 2003 20:27:47
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_SKY_H
#define INCLUSIONGUARD_SKY_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class Scene;

//------------------------------------------------------------------------------
// Name: class Sky
// Desc: The sky mesh object
//------------------------------------------------------------------------------
class Sky
{
public:
	Sky();

	HRESULT InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice, const bool dx9Shaders,
							   const bool twoSidedStencil );
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT DeleteDeviceObjects();

	HRESULT Render( const Scene& scene );

	inline void SetTime( const float time ) { m_time = time; }

private:
	const static float FAR_PLANE;
	const static int SKYPLANE_DIM = 20;
	const static int NUM_VERTS = SKYPLANE_DIM * SKYPLANE_DIM;
	const static int NUM_FACES = ( SKYPLANE_DIM - 1 ) * ( SKYPLANE_DIM - 1 ) * 2;

	HRESULT CreateSkyPlane();

	float m_time;

	//direct3d objects
	struct TerrainVertex;
	LPDIRECT3DDEVICE9		m_pd3dDevice;
	LPD3DXMESH				m_pMesh;
	LPDIRECT3DVERTEXBUFFER9	m_pVB;
	DWORD					m_numVertices;
	LPDIRECT3DINDEXBUFFER9	m_pIB;
	DWORD					m_numFaces;
	D3DXVECTOR3				m_vPosition;
	LPDIRECT3DVERTEXSHADER9 m_pVS;
	LPDIRECT3DVERTEXDECLARATION9 m_pVSDecl;
	LPDIRECT3DPIXELSHADER9	m_pPS;
	LPDIRECT3DTEXTURE9		m_pCloudTexture;

};


#endif //INCLUSIONGUARD_SKY_H
