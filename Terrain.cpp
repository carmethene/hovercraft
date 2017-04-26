//------------------------------------------------------------------------------
// File: Terrain.cpp
// Desc: Heightmapped terrain with quadtree-based frustum culling
//
// Created: 02 January 2003 13:47:18
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <new>

#include "Terrain.h"
#include "Frustum.h"
#include "Resource.h"
#include "Scene.h"

#include "DXUtil.h"


//------------------------------------------------------------------------------
// Constants:
//------------------------------------------------------------------------------
const float Terrain::TERRAIN_SCALE = 4.0f;


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: struct TerrainVertex
// Desc: A single vertex in the terrain mesh
//------------------------------------------------------------------------------
struct Terrain::TerrainVertex
{
	D3DXVECTOR3 p;		//untransformed vertex position
	D3DXVECTOR3 n;		//untransformed vertex normal
	D3DCOLOR diffuse;	//difuse vertex colour - used for texture blending
	float tu1, tv1;		//texture coordinates
	float tu2, tv2;
};

//------------------------------------------------------------------------------
// Name: Terrain()
// Desc: Constructor for the terrain object
//------------------------------------------------------------------------------
Terrain::Terrain()
{
	//initialise member vars
	m_pVSAmbient = NULL;
	m_pVSDiffuse = NULL;
	m_pVSDecl	 = NULL;
	m_pPS		 = NULL;

	m_pVB			= NULL;
	m_pIB			= NULL;
	m_numVertices	= 0;
	m_numFaces		= 0;

	m_pTextureFlat	= NULL;
	m_pTextureSlope	= NULL;

	//create the terrain heightmap
	GenerateHeightmap();

	//create the terrain quadtree
	m_pQuadtree = NULL;
	BuildQuadtree();
}

//------------------------------------------------------------------------------
// Name: ~Terrain()
// Desc: Destructor for the terrain object
//------------------------------------------------------------------------------
Terrain::~Terrain()
{
	//destroy the terrain quadtree
	delete m_pQuadtree;
	m_pQuadtree = NULL;
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT Terrain::InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice,
								    const bool dx9Shaders,
									const bool twoSidedStencil )
{
	UNREFERENCED_PARAMETER( twoSidedStencil );

	//store a local copy of the direct3d device
	m_pd3dDevice = pd3dDevice;

	//create the buffers
	OutputDebugString( "Creating terrain buffers..." );

	const int VB_SIZE = NUM_VERTS * sizeof( TerrainVertex );
	if( FAILED( m_pd3dDevice->CreateVertexBuffer( VB_SIZE, D3DUSAGE_WRITEONLY, 0,
												  D3DPOOL_MANAGED, &m_pVB, NULL ) ) )
		return E_FAIL;
	
	const int IB_SIZE = FACES_PER_CELL * 3 * sizeof( WORD );
	if( FAILED( m_pd3dDevice->CreateIndexBuffer( IB_SIZE, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
												 D3DPOOL_MANAGED, &m_pIB, NULL ) ) )
		return E_FAIL;

	//create the textures
	if( FAILED( D3DXCreateTextureFromResource( m_pd3dDevice, NULL,
				MAKEINTRESOURCE( IDD_TEX_TERRAINFLAT ), &m_pTextureFlat ) ) )
		return E_FAIL;
	if( FAILED( D3DXCreateTextureFromResource( m_pd3dDevice, NULL,
				MAKEINTRESOURCE( IDD_TEX_TERRAINSLOPE ), &m_pTextureSlope ) ) )
		return E_FAIL;

	OutputDebugString( "done\n" );

	//fill out the terrain buffers
	if( FAILED( FillVertexBuffer() ) )
		return E_FAIL;
	if( FAILED( FillIndexBuffer() ) )
		return E_FAIL;

	OutputDebugString( "Creating terrain shaders..." );

	//define the vertex shader
	D3DVERTEXELEMENT9 vsDecl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 36, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END(),
	};
	
	if( FAILED( m_pd3dDevice->CreateVertexDeclaration( vsDecl, &m_pVSDecl ) ) )
		return E_FAIL;

	//create the vertex shaders
	LPD3DXBUFFER pCode, pErrors;
	DWORD flags = 0;
	#if defined(_DEBUG) || defined(DEBUG)
	flags |= D3DXSHADER_DEBUG;
	#endif

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_TERRAIN_AMBIENT ),
												NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (terrain ambient), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		return E_FAIL;
	}

    if( FAILED( m_pd3dDevice->CreateVertexShader( (const DWORD*)pCode->GetBufferPointer(),
												  &m_pVSAmbient ) ) )
		return E_FAIL;

	SAFE_RELEASE( pErrors );
	SAFE_RELEASE( pCode );

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_TERRAIN_DIFFUSE ),
												NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (terrain diffuse), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		return E_FAIL;
	}

    if( FAILED( m_pd3dDevice->CreateVertexShader( (const DWORD*)pCode->GetBufferPointer(),
												  &m_pVSDiffuse ) ) )
		return E_FAIL;

	SAFE_RELEASE( pErrors );
	SAFE_RELEASE( pCode );

	//create the pixel shader
	if( dx9Shaders )
	{
		if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_PS_TERRAIN ),
													NULL, NULL, flags, &pCode, &pErrors ) ) )
		{
			OutputDebugString( "Failed to assemble pixel shader (terrain), errors:\n" );
			OutputDebugString( (char*)pErrors->GetBufferPointer() );
			OutputDebugString( "\n" );

			return E_FAIL;
		}

		if( FAILED( m_pd3dDevice->CreatePixelShader( (const DWORD*)pCode->GetBufferPointer(),
													&m_pPS ) ) )
			return E_FAIL;

		SAFE_RELEASE( pErrors );
		SAFE_RELEASE( pCode );
	}

	OutputDebugString( "done\n" );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Sets up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT Terrain::RestoreDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT Terrain::InvalidateDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT Terrain::DeleteDeviceObjects()
{
	//delete the textures
	SAFE_RELEASE( m_pTextureFlat );
	SAFE_RELEASE( m_pTextureSlope );

	//delete the mesh data
	SAFE_RELEASE( m_pVB );
	SAFE_RELEASE( m_pIB );
	m_numFaces = 0;
	m_numVertices = 0;

	//destroy the shaders
	SAFE_RELEASE( m_pVSAmbient );
	SAFE_RELEASE( m_pVSDiffuse );
	SAFE_RELEASE( m_pVSDecl );
	SAFE_RELEASE( m_pPS );

	m_pd3dDevice = NULL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the object
//------------------------------------------------------------------------------
HRESULT Terrain::Render( const Scene& scene, const bool useLight ) const
{
	//set vertex shader constants...
	//transform matrix
	D3DXMATRIX matViewProj = scene.GetCamera().GetViewProj();
	D3DXMATRIX matViewProjTranspose;
	D3DXMatrixTranspose( &matViewProjTranspose, &matViewProj );
	m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&matViewProjTranspose, 4 );

	//which lighting mode are we using?
	if( useLight )
	{
		//diffuse
		D3DXVECTOR4 vLightDirection = scene.GetLight( 0 ).GetPosition();
		m_pd3dDevice->SetVertexShaderConstantF( 4, (float*)&vLightDirection, 1 );
		D3DXVECTOR4 vLightColour = scene.GetLight( 0 ).GetColour();
		m_pd3dDevice->SetVertexShaderConstantF( 5, (float*)&vLightColour, 1 );

		m_pd3dDevice->SetVertexShader( m_pVSDiffuse );
	}
	else
	{
		//ambient
		D3DXVECTOR4 vAmbientColour = scene.GetAmbientLight();
		m_pd3dDevice->SetVertexShaderConstantF( 4, (float*)&vAmbientColour, 1 );

		m_pd3dDevice->SetVertexShader( m_pVSAmbient );
	}

	//set device parameters
	m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof( TerrainVertex ) );
	m_pd3dDevice->SetIndices( m_pIB );
	m_pd3dDevice->SetVertexDeclaration( m_pVSDecl );
	m_pd3dDevice->SetPixelShader( m_pPS );
	m_pd3dDevice->SetTexture( 0, m_pTextureFlat );
	m_pd3dDevice->SetTexture( 1, m_pTextureSlope );

	//render all visible nodes
	std::vector< unsigned int >::const_iterator iter = m_visibleCells.begin();
	while( iter != m_visibleCells.end() )
	{
		m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, *iter, 0, VERTS_PER_CELL,
											0, FACES_PER_CELL );
		iter++;
	}

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: CullQuadtree()
// Desc: Finds visible nodes in the quadtree
//------------------------------------------------------------------------------
HRESULT Terrain::CullQuadtree( const Scene& scene )
{
	D3DXMATRIX matViewProj = scene.GetCamera().GetViewProj();
	Frustum frustum = ExtractFrustum( matViewProj, false );

	m_visibleCells.clear();

	m_pQuadtree->AddVisibleNodes( frustum, m_visibleCells );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: GetHeightMapPoint()
// Desc: Retrieves an interpolated value for the height at a given point
//------------------------------------------------------------------------------
float Terrain::GetHeightMapPoint( const float xPos, const float zPos ) const
{
	const float x = xPos / TERRAIN_SCALE;
	const float z = zPos / TERRAIN_SCALE;

	//find current square
	const float minX = float( floor( x ) );
	const float minZ = float( floor( z ) );
	int intX = int( minX );
	int intZ = int( minZ );

	//make sure values are within range of the heightmap
	if( intX < 0 ) intX = 0;
	if( intX >= ( HEIGHTMAP_DIM - 1 ) ) intX = ( HEIGHTMAP_DIM - 2 );
	if( intZ < 0 ) intZ = 0;
	if( intZ >= ( HEIGHTMAP_DIM - 1 ) ) intZ = ( HEIGHTMAP_DIM - 2 );

	//find weights
	const float wx = x - minX;
	const float wz = z - minZ;

	//get surrounding points
	const float p11 = GetHeightMapPoint( intX, intZ );
	const float p12 = GetHeightMapPoint( intX, intZ + 1 );
	const float p21 = GetHeightMapPoint( intX + 1, intZ );
	const float p22 = GetHeightMapPoint( intX + 1, intZ + 1 );

	//lerp in x direction
	const float px1 = p11 + wx * ( p21 - p11 );
	const float px2 = p12 + wx * ( p22 - p12 );

	//lerp in z direction
	const float p = px1 + wz * ( px2 - px1 );
	
	return p;
}

//------------------------------------------------------------------------------
// Name: GenerateHeightmap()
// Desc: Fills the heightmap with height values based on a given method
//------------------------------------------------------------------------------
void Terrain::GenerateHeightmap()
{
	OutputDebugString( "Generating terrain heightmap..." );

	//fill the heightfield with perlin noise
	for( int row = 0; row < HEIGHTMAP_DIM; ++row )
	{
		for( int column = 0; column < HEIGHTMAP_DIM; ++column )
		{
			int index = column + ( row * HEIGHTMAP_DIM );
			m_heights[ index ] = PerlinNoise2D( float( column ), float( row ) );
		}
	}

	OutputDebugString( "done\n" );
}

//------------------------------------------------------------------------------
// Name: FillVertexBuffer()
// Desc: Fills a vertex buffer with the terrain vertices
//------------------------------------------------------------------------------
HRESULT Terrain::FillVertexBuffer()
{
	OutputDebugString( "Creating terrain geometry (vertices)..." );

	//lock the vertex buffer
	TerrainVertex* pBuffer = NULL;
	if( FAILED( m_pVB->Lock( 0, NUM_VERTS * sizeof( TerrainVertex ),
							 (void**)&pBuffer, 0 ) ) )
		return E_FAIL;

	int bufferIndex = 0;

	//create the vertices...
	//for each cell
	for( int cellColumn = 0; cellColumn < CELLS_DIM; ++cellColumn )
	{
		for( int cellRow = 0; cellRow < CELLS_DIM; ++cellRow )				
		{
			for( int subRow = 0; subRow <= QuadtreeNode::LEAFNODE_WIDTH; ++subRow )
			{
				for( int subColumn = 0; subColumn <= QuadtreeNode::LEAFNODE_WIDTH; ++subColumn )
				{
					int row = subRow + ( cellRow * QuadtreeNode::LEAFNODE_WIDTH );
					int column = subColumn + ( cellColumn * QuadtreeNode::LEAFNODE_WIDTH );
					int index = column + ( row * HEIGHTMAP_DIM );

					//convert to floating point once, as we will need this many times
					float fRow		= float( row ) * TERRAIN_SCALE;
					float fColumn	= float( column ) * TERRAIN_SCALE;

					//calculate the position of this vertex
					D3DXVECTOR3 vPosition = D3DXVECTOR3( fRow,
														 m_heights[ index ],
														 fColumn );

					//calculate the normal for this vertex
					D3DXVECTOR3 vNormal = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

					//upper left face...
					if( column != 0 && row != 0 )
					{
						D3DXVECTOR3 v1 = vPosition;
						D3DXVECTOR3 v2 = D3DXVECTOR3( fRow,
													m_heights[ index - 1 ],
													fColumn - 1.0f );
						D3DXVECTOR3 v3 = D3DXVECTOR3( fRow - 1.0f,
													m_heights[ index - HEIGHTMAP_DIM ],
													fColumn );

						vNormal += GetFaceNormal( v1, v2, v3 );
					}

					//upper right face...
					if( column != ( HEIGHTMAP_DIM - 1 ) && row != 0 )
					{
						D3DXVECTOR3 v1 = vPosition;
						D3DXVECTOR3 v2 = D3DXVECTOR3( fRow - 1.0f,
													m_heights[ index - HEIGHTMAP_DIM ],
													fColumn );
						D3DXVECTOR3 v3 = D3DXVECTOR3( fRow,
													m_heights[ index + 1 ],
													fColumn + 1.0f );

						vNormal += GetFaceNormal( v1, v2, v3 );
					}

					//lower left face...
					if( column != 0 && row != ( HEIGHTMAP_DIM - 1 ) )
					{
						D3DXVECTOR3 v1 = vPosition;
						D3DXVECTOR3 v2 = D3DXVECTOR3( fRow,
													m_heights[ index - 1 ],
													fColumn - 1.0f );
						D3DXVECTOR3 v3 = D3DXVECTOR3( fRow - 1.0f,
													m_heights[ index + HEIGHTMAP_DIM ],
													fColumn );

						vNormal += GetFaceNormal( v1, v2, v3 );
					}

					//lower right face...
					if( column != ( HEIGHTMAP_DIM - 1 ) && row != ( HEIGHTMAP_DIM - 1 ) )
					{
						D3DXVECTOR3 v1 = vPosition;
						D3DXVECTOR3 v2 = D3DXVECTOR3( fRow,
													m_heights[ index + 1 ],
													fColumn + 1.0f );
						D3DXVECTOR3 v3 = D3DXVECTOR3( fRow + 1.0f,
													m_heights[ index + HEIGHTMAP_DIM ],
													fColumn );

						vNormal += GetFaceNormal( v1, v2, v3 );
					}

					//normalize result
					D3DXVec3Normalize( &vNormal, &vNormal );

					//calculate blending value based on height
					int blendValue = int( ( m_heights[ index ] + 8.0f ) * 20.0f );

					//cap
					if( blendValue > 255 )
						blendValue = 255;
					else if( blendValue < 0 )
						blendValue = 0;

					//create the vertex description
					TerrainVertex v;
					v.p = vPosition;
					v.n = vNormal;
					v.diffuse = D3DCOLOR_ARGB( blendValue, 255, 255, 255 );

					const static int terrainScale = int( TERRAIN_SCALE );
					float texRow = float( row * terrainScale );
					texRow /= 16.0f;
					float texCol = float( column * terrainScale );
					texCol /= 16.0f;
	
					v.tu1 = texRow;
					v.tv1 = texCol;
					v.tu2 = texRow;
					v.tv2 = texCol;

					//store the vertex in the buffer
					pBuffer[ bufferIndex++ ] = v;
				}
			}
		}
	}

	//unlock the vertex buffer
	m_pVB->Unlock();

	OutputDebugString( "done\n" );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: FillIndexBuffer
// Desc: Fills an index buffer with indices necessary to render the a single
//		 cell in the terrain
//------------------------------------------------------------------------------
HRESULT Terrain::FillIndexBuffer()
{	
	OutputDebugString( "Creating terrain geometry (indices)..." );

	//lock the index buffer
	WORD* pBuffer = NULL;
	int bufferIndex = 0;
	if( FAILED( m_pIB->Lock( 0, FACES_PER_CELL * 3 * sizeof( WORD ),
							 (void**)&pBuffer, 0 ) ) )
		return E_FAIL;

	const int realWidth = QuadtreeNode::LEAFNODE_WIDTH + 1;

	//for each quad in the cell
	for( int row = 0; row < QuadtreeNode::LEAFNODE_WIDTH; ++row )
	{
		for( int column = 0; column < QuadtreeNode::LEAFNODE_WIDTH; ++column )
		{
			//create triangles for this quad
			WORD firstIndex = WORD( column + ( row * realWidth ) );

			//triangle 1
			pBuffer[ bufferIndex++ ] = firstIndex;
			pBuffer[ bufferIndex++ ] = firstIndex + 1;
			pBuffer[ bufferIndex++ ] = firstIndex + realWidth;

			//triangle 2
			pBuffer[ bufferIndex++ ] = firstIndex + realWidth;
			pBuffer[ bufferIndex++ ] = firstIndex + 1;
			pBuffer[ bufferIndex++ ] = firstIndex + realWidth + 1;
		}
	}

	//unlock the index buffer
	m_pIB->Unlock();

	OutputDebugString( "done\n" );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: BuildQuadtree()
// Desc: Creates a quadtree for the terrain
//------------------------------------------------------------------------------
HRESULT Terrain::BuildQuadtree()
{
	OutputDebugString( "Creating terrain quadtree..." );

	const float minY = -100.0f;
	const float maxY = 100.0f;
	const float cellWidth = float( QuadtreeNode::LEAFNODE_WIDTH );
	const int verticesPerCell = ( QuadtreeNode::LEAFNODE_WIDTH + 1 ) *
								( QuadtreeNode::LEAFNODE_WIDTH + 1 );

	//create leaf nodes...
	QuadtreeNode** pNodes = NULL;
	try{ pNodes = new QuadtreeNode*[ CELLS_DIM * CELLS_DIM ]; }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		exit( 1 );
	}

	//for each cell - note, loop ordering is important here as aabb must match with vertices
	for( int cellColumn = 0; cellColumn < CELLS_DIM; ++cellColumn )
	{
		for( int cellRow = 0; cellRow < CELLS_DIM; ++cellRow )			
		{
			const float fCellRow = float( cellRow );
			const float fCellColumn = float( cellColumn );

			//calculate x and z bounding values
			const float minX = fCellColumn * cellWidth * TERRAIN_SCALE;
			const float maxX = minX + cellWidth * TERRAIN_SCALE;
			const float minZ = fCellRow * cellWidth * TERRAIN_SCALE;
			const float maxZ = minZ + cellWidth * TERRAIN_SCALE;

			//calculate base vertex for this cell
			const int cellNumber = cellColumn + ( cellRow * CELLS_DIM );
			const int baseVertex = verticesPerCell * cellNumber;

			//create the quadtree leaf node for this cell
			QuadtreeNode* pNode = new QuadtreeNode( minX, maxX, minY, maxY, minZ, maxZ,
													baseVertex );
			pNodes[ ( cellRow * CELLS_DIM ) + cellColumn ] = pNode;
		}
	}

    //build quadtree from leaf nodes...
	int level = CELLS_DIM / 2;
	QuadtreeNode** pNewNodes = NULL;

	for(;;)	//forever
	{
		try{ pNewNodes = new QuadtreeNode*[ level * level ]; }
		catch( std::bad_alloc& error )
		{
			MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
			exit( 1 );
		}		

		//for each node at this level
		for( int row = 0; row < level; ++row )
		{
			for( int column = 0; column < level; ++column )
			{
                //find child nodes
				const int lastLevel = level * 2;
				const int rowIndex = row * 2;
				const int columnIndex = column * 2;

				QuadtreeNode* child1 = pNodes[ ( rowIndex * lastLevel ) + columnIndex ];
				QuadtreeNode* child2 = pNodes[ ( rowIndex * lastLevel ) + columnIndex + 1 ];				
				QuadtreeNode* child3 = pNodes[ ( (rowIndex + 1)* lastLevel ) + columnIndex ];
				QuadtreeNode* child4 = pNodes[ ( (rowIndex + 1)* lastLevel ) + columnIndex + 1 ];

				//calculate bounding values
				float minX = min( child1->GetAABBMin( 0 ),
							  min( child2->GetAABBMin( 0 ),
							   min( child3->GetAABBMin( 0 ),
									child4->GetAABBMin( 0 ) ) ) );
				float maxX = max( child1->GetAABBMax( 0 ),
							  max( child2->GetAABBMax( 0 ),
							   max( child3->GetAABBMax( 0 ),
									child4->GetAABBMax( 0 ) ) ) );
				
				float minY = child1->GetAABBMin( 1 );
				float maxY = child1->GetAABBMax( 1 );

				float minZ = min( child1->GetAABBMin( 2 ),
							  min( child2->GetAABBMin( 2 ),
							   min( child3->GetAABBMin( 2 ),
									child4->GetAABBMin( 2 ) ) ) );
				float maxZ = max( child1->GetAABBMax( 2 ),
							  max( child2->GetAABBMax( 2 ),
							   max( child3->GetAABBMax( 2 ),
									child4->GetAABBMax( 2 ) ) ) );

				//create parent node and store
				QuadtreeNode* pNode = new QuadtreeNode( child1, child2, child3, child4,
														minX, maxX, minY, maxY, minZ, maxZ );
				pNewNodes[ ( row * level ) + column ] = pNode;
			}
		}
		
		//update the node store
		delete[] pNodes;
		pNodes = pNewNodes;

		//are we at the root level?
		if( level == 1 )
			break;

		level /= 2;
	}

	//store the parent node
	m_pQuadtree = pNodes[ 0 ];
	delete[] pNodes;

	OutputDebugString( "done\n" );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: GetFaceNormal()
// Desc: Returns the face normal of a given triangle
//------------------------------------------------------------------------------
D3DXVECTOR3 Terrain::GetFaceNormal( const D3DXVECTOR3& v1, const D3DXVECTOR3& v2,
									const D3DXVECTOR3& v3 ) const
{
	D3DXVECTOR3 e1 = v2 - v1;
	D3DXVECTOR3 e2 = v3 - v2;
	D3DXVECTOR3 vNormal;

	D3DXVec3Cross( &vNormal, &e1, &e2 );
	D3DXVec3Normalize( &vNormal, &vNormal );

	return vNormal;
}

//------------------------------------------------------------------------------
// Name: Interpolate()
// Desc: Interpolates between two values using a cosine interpolation scheme
//------------------------------------------------------------------------------
float Terrain::Interpolate( const float a, const float b, const float x ) const
{
	float ft = x * D3DX_PI;
	ft = ( 1.0f - float( cos(ft) ) ) * 0.5f;

	return a * ( 1.0f - ft ) + b * ft;
}

//------------------------------------------------------------------------------
// Name: Noise1()
// Desc: Generates integer noise
//------------------------------------------------------------------------------
float Terrain::Noise1( const int x, const int y ) const
{
	int n = x + y * 57;
	n = ( n << 13 ) ^ n;
	return ( 1.0f - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff )
			/ 1073741824.0f );    
}

//------------------------------------------------------------------------------
// Name: SmoothedNoise1()
// Desc: Generates smoothed integer noise
//------------------------------------------------------------------------------
float Terrain::SmoothedNoise1( const int x, const int y ) const
{
	float corners = Noise1( x - 1, y - 1 ) +
					Noise1( x - 1, y + 1 ) +
					Noise1( x + 1, y - 1 ) +
					Noise1( x + 1, y + 1 );
	corners /= 16.0f;

	float sides = Noise1( x - 1, y ) +
				  Noise1( x + 1, y ) +
				  Noise1( x, y - 1 ) +
				  Noise1( x, y + 1 );
	sides /= 8.0f;

	float center = Noise1( x, y );
	center /= 4.0f;

    return corners + sides + center;
}

//------------------------------------------------------------------------------
// Name: InterpolatedNoise1()
// Desc: Generates interpolated noise
//------------------------------------------------------------------------------
float Terrain::InterpolatedNoise1( const float x, const float y ) const
{
	int integer_X = int( x );
	float fractional_X = x - float( integer_X );
	int integer_Y = int( y );
	float fractional_Y = y - float( integer_Y );

	float v1 = SmoothedNoise1( integer_X, integer_Y );
	float v2 = SmoothedNoise1( integer_X + 1, integer_Y );
	float v3 = SmoothedNoise1( integer_X, integer_Y + 1 );
	float v4 = SmoothedNoise1( integer_X + 1, integer_Y + 1 );

	float i1 = Interpolate( v1, v2, fractional_X );
	float i2 = Interpolate( v3, v4, fractional_X );

	return Interpolate( i1, i2, fractional_Y );
}

//------------------------------------------------------------------------------
// Name: PerlinNoise2D()
// Desc: Generates 2-dimensional Perlin noise
//------------------------------------------------------------------------------
float Terrain::PerlinNoise2D( const float x, const float y ) const
{
	//these values generate very nice noise (trial and error)
	float total = InterpolatedNoise1( x * 0.05f, y * 0.05f ) * 60.0f;
	total += InterpolatedNoise1( x * 0.1f, y * 0.1f ) * 80.0f;
	total += InterpolatedNoise1( x * 0.5f, y * 0.5f ) * 5.0f;

	return total;
}
