//------------------------------------------------------------------------------
// File: Terrain.h
// Desc: Heightmapped terrain with quadtree-based frustum culling
//
// Created: 02 January 2003 13:39:01
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_TERRAIN_H
#define INCLUSIONGUARD_TERRAIN_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <vector>
#include <d3dx9.h>

#include "QuadtreeNode.h"


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class Scene;

//------------------------------------------------------------------------------
// Name: class Terrain
// Desc: The heightmapped terrain object
//------------------------------------------------------------------------------
class Terrain
{
public:
	//cells per edge - must be a power of two
	#if defined(_DEBUG) || defined(DEBUG)
	const static int CELLS_DIM = 4;	//heightmap generation is slow for debug builds
	#else
	const static int CELLS_DIM = 32;
	#endif

	Terrain();
	~Terrain();

	HRESULT InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice, const bool dx9Shaders,
							   const bool twoSidedStencil );
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT DeleteDeviceObjects();

	HRESULT Render( const Scene& scene, const bool useLight ) const;
	HRESULT CullQuadtree( const Scene& scene );

	float GetHeightMapPoint( const float xPos, const float zPos ) const;
	float GetTerrainSize() const { return (HEIGHTMAP_DIM - 1) * TERRAIN_SCALE; }

	unsigned int GetVisibleCells() const
	{
		return static_cast<unsigned int>( m_visibleCells.size() );
	}

private:
	const static int HEIGHTMAP_DIM = ( CELLS_DIM * QuadtreeNode::LEAFNODE_WIDTH ) + 1;
	const static int FACES_PER_CELL = QuadtreeNode::LEAFNODE_WIDTH *
									  QuadtreeNode::LEAFNODE_WIDTH * 2;
	const static int VERTS_PER_CELL = ( QuadtreeNode::LEAFNODE_WIDTH + 1 ) *
									  ( QuadtreeNode::LEAFNODE_WIDTH + 1 );
	const static int NUM_VERTS = VERTS_PER_CELL * CELLS_DIM * CELLS_DIM;
	const static float TERRAIN_SCALE;

	inline float GetHeightMapPoint( const int x, const int z ) const
	{
		return m_heights[ z + ( x * HEIGHTMAP_DIM ) ];
	}

	void GenerateHeightmap();

	HRESULT FillVertexBuffer();
	HRESULT FillIndexBuffer();
	HRESULT BuildQuadtree();

    D3DXVECTOR3 GetFaceNormal( const D3DXVECTOR3& v1, const D3DXVECTOR3& v2,
							   const D3DXVECTOR3& v3 ) const;

	//perlin noise generator functions
	inline float Interpolate( const float a, const float b, const float x ) const;
	inline float Noise1( const int x, const int y ) const;
	inline float SmoothedNoise1( const int x, const int y ) const;
	inline float InterpolatedNoise1( const float x, const float y ) const;
	float PerlinNoise2D( const float x, const float y ) const;

	//heightmap
	float m_heights[ HEIGHTMAP_DIM * HEIGHTMAP_DIM ];

	//terrain quadtree
	QuadtreeNode* m_pQuadtree;
	std::vector<unsigned int> m_visibleCells;

	//direct3d objects
	struct TerrainVertex;
	LPDIRECT3DDEVICE9		m_pd3dDevice;
	LPD3DXMESH				m_pMesh;
	LPDIRECT3DVERTEXBUFFER9	m_pVB;
	DWORD					m_numVertices;
	LPDIRECT3DINDEXBUFFER9	m_pIB;
	DWORD					m_numFaces;
	D3DXVECTOR3				m_vPosition;
	LPDIRECT3DTEXTURE9		m_pTextureFlat;
	LPDIRECT3DTEXTURE9		m_pTextureSlope;
	LPDIRECT3DVERTEXSHADER9 m_pVSAmbient;
	LPDIRECT3DVERTEXSHADER9 m_pVSDiffuse;
	LPDIRECT3DVERTEXDECLARATION9 m_pVSDecl;
	LPDIRECT3DPIXELSHADER9	m_pPS;

};


#endif //INCLUSIONGUARD_TERRAIN_H
