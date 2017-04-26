//------------------------------------------------------------------------------
// File: ShadowVolume.h
// Desc: A representation of a shadow volume along with methods for generating
//		 and rendering it
//
// Created: 09 January 2003 13:52:53
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_SHADOWVOLUME_H
#define INCLUSIONGUARD_SHADOWVOLUME_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: class ShadowVolume
// Desc: The shadow volume object
//-----------------------------------------------------------------------------
class ShadowVolume
{
public:
	ShadowVolume();

	HRESULT InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice, const bool dx9Shaders,
							   const bool twoSidedStencil );
	HRESULT DeleteDeviceObjects();
	HRESULT Render() const;

	void Reset() { m_numVertices = 0; }
	HRESULT BuildFromMesh( const LPD3DXMESH pMesh, const D3DXVECTOR3 vLight );

	void ShowVolumes( const bool showVolumes ) { m_showVolumes = showVolumes; }

private:
	bool m_showVolumes;
	bool m_twoSidedStencil;

	D3DXVECTOR3 m_pVertices[ 32000 ];
	DWORD m_numVertices;

	//direct3d objects
	struct TerrainVertex;
	LPDIRECT3DDEVICE9		m_pd3dDevice;
	LPDIRECT3DVERTEXSHADER9 m_pVS;
	LPDIRECT3DVERTEXDECLARATION9 m_pVSDecl;

};


#endif //INCLUSIONGUARD_SHADOWVOLUME_H
