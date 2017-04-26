//------------------------------------------------------------------------------
// File: Backdrop.h
// Desc: Screen-aligned quad with a backdrop texture
//
// Created: 05 January 2003 16:23:49
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_BACKDROP_H
#define INCLUSIONGUARD_BACKDROP_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class Scene;

//------------------------------------------------------------------------------
// Name: class Backdrop
// Desc: The backdrop object
//------------------------------------------------------------------------------
class Backdrop
{
public:
	Backdrop();

	HRESULT InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice, const bool dx9Shaders,
							   const bool twoSidedStencil );
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT DeleteDeviceObjects();

	HRESULT Render( const Scene& scene );

private:
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
	LPDIRECT3DTEXTURE9		m_pBackdropTexture;

};


#endif //INCLUSIONGUARD_BACKDROP_H
