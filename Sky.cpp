//------------------------------------------------------------------------------
// File: Sky.cpp
// Desc: Sky mesh with procedural cloud shader
//
// Created: 04 January 2003 20:33:24
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <cmath>

#include "Sky.h"
#include "Resource.h"
#include "Scene.h"

#include "DXUtil.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------
const float Sky::FAR_PLANE = 350.0f;

//------------------------------------------------------------------------------
// Name: struct SkyplaneVertex
// Desc: A single vertex in the skyplane
//------------------------------------------------------------------------------
struct SkyplaneVertex
{
	D3DXVECTOR3 p;
};

//------------------------------------------------------------------------------
// Name: Sky()
// Desc: Constructor for the sky object
//------------------------------------------------------------------------------
Sky::Sky()
{
	//initialise member variables
	m_pd3dDevice = NULL;
	m_pVB		 = NULL;
	m_pIB		 = NULL;

	m_pVS		 = NULL;
	m_pVSDecl	 = NULL;
	m_pPS		 = NULL;

	m_pCloudTexture = NULL;

	//initialise position
	m_vPosition	= D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT Sky::InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice,
							    const bool dx9Shaders,
								const bool twoSidedStencil )
{
	UNREFERENCED_PARAMETER( twoSidedStencil );

	//store a local copy of the direct3d device
	m_pd3dDevice = pd3dDevice;

	//create the skyplane buffers
	OutputDebugString( "Creating skyplane shaders..." );

	const int VB_SIZE = NUM_VERTS * sizeof( SkyplaneVertex );
	if( FAILED( m_pd3dDevice->CreateVertexBuffer( VB_SIZE, D3DUSAGE_WRITEONLY, 0,
												  D3DPOOL_MANAGED, &m_pVB, NULL ) ) )
		return E_FAIL;
	
	const int IB_SIZE = NUM_FACES * 3 * sizeof( WORD );
	if( FAILED( m_pd3dDevice->CreateIndexBuffer( IB_SIZE, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
												 D3DPOOL_MANAGED, &m_pIB, NULL ) ) )
		return E_FAIL;

	//create the cloud texture
	if( FAILED( D3DXCreateTextureFromResource( m_pd3dDevice, NULL,
				MAKEINTRESOURCE( IDD_TEX_CLOUDTURB ), &m_pCloudTexture ) ) )
		return E_FAIL;

	OutputDebugString( "done\n" );

	//fill the skyplane buffers
	CreateSkyPlane();

	OutputDebugString( "Creating skyplane shaders..." );

	//define the vertex shader
	D3DVERTEXELEMENT9 vsDecl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END(),
	};
	
	if( FAILED( m_pd3dDevice->CreateVertexDeclaration( vsDecl, &m_pVSDecl ) ) )
		return E_FAIL;

	//create the vertex shader objects
	LPD3DXBUFFER pCode, pErrors;
	DWORD flags = 0;
	#if defined(_DEBUG) || defined(DEBUG)
	flags |= D3DXSHADER_DEBUG;
	#endif

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_SKY ),
											NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader, errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		return E_FAIL;
	}

    if( FAILED( m_pd3dDevice->CreateVertexShader( (const DWORD*)pCode->GetBufferPointer(),
												  &m_pVS ) ) )
		return E_FAIL;

	SAFE_RELEASE( pErrors );
	SAFE_RELEASE( pCode );

	//create the pixel shader
	if( dx9Shaders )
	{
		if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_PS_SKY ),
													NULL, NULL, flags, &pCode, &pErrors ) ) )
		{
			OutputDebugString( "Failed to assemble pixel shader, errors:\n" );
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
HRESULT Sky::RestoreDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT Sky::InvalidateDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT Sky::DeleteDeviceObjects()
{
	//destroy the noise texture
	SAFE_RELEASE( m_pCloudTexture );

	//destroy the buffers
	SAFE_RELEASE( m_pVB );
	SAFE_RELEASE( m_pIB );

	//destroy the shaders
	SAFE_RELEASE( m_pVS );
	SAFE_RELEASE( m_pVSDecl );
	SAFE_RELEASE( m_pPS );

	m_pd3dDevice = NULL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the object
//------------------------------------------------------------------------------
HRESULT Sky::Render( const Scene& scene )
{
	//set vertex shader constants...
	//transform matrix
	D3DXMATRIX matViewProj = scene.GetCamera().GetViewProj();	

	//camera should be at the center of the sky
	D3DXMATRIX matWorld;
	const D3DXVECTOR3& vCamPosition = scene.GetCamera().GetPosition();
	D3DXMatrixTranslation( &matWorld, vCamPosition[0], vCamPosition[1], vCamPosition[2] );

	//set the transform matrix
	D3DXMATRIX matResult;
	D3DXMatrixMultiplyTranspose( &matResult, &matWorld, &matViewProj );
	m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&matResult, 4 );

	//set the current time
	D3DXVECTOR4 c4( (float)m_time * 0.02085f, (float)m_time * 0.002515f, 0.0f, 0.0f );		
	m_pd3dDevice->SetVertexShaderConstantF( 4, (float*)c4, 1 );

	//set device parameters
	m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof( SkyplaneVertex ) );
	m_pd3dDevice->SetIndices( m_pIB );
	m_pd3dDevice->SetVertexDeclaration( m_pVSDecl );
	m_pd3dDevice->SetVertexShader( m_pVS );
	m_pd3dDevice->SetPixelShader( m_pPS );
	m_pd3dDevice->SetTexture( 0, m_pCloudTexture );

	//draw the object
	m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, NUM_VERTS, 0, NUM_FACES );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: CreateSkyPlane()
// Desc: Creates a skyplane
//------------------------------------------------------------------------------
HRESULT Sky::CreateSkyPlane()
{
	OutputDebugString( "Creating skyplane geometry..." );

	//lock the vertex buffer
	SkyplaneVertex* pBuffer = NULL;
	if( FAILED( m_pVB->Lock( 0, NUM_VERTS * sizeof( SkyplaneVertex ),
							 (void**)&pBuffer, 0 ) ) )
		return E_FAIL;

	int bufferIndex = 0;

	const float halfSize = FAR_PLANE * 1.5f;
	const float minY = 5.0f;
	const float centerHeight = 30.0f;
	const float horizonHeight = centerHeight - minY;

	const float CellWidth = ( 2.0f * halfSize ) / ( SKYPLANE_DIM - 1 );

	//create the vertices
	for( int row = 0; row < SKYPLANE_DIM; ++row )
	{
		for( int column = 0; column < SKYPLANE_DIM; ++column )
		{
			float x = ( CellWidth * float( column ) ) - halfSize;
			float z = ( CellWidth * float( row ) ) - halfSize;			

			const float absX = ( x >= 0 ) ? x : -x;
			const float absZ = ( z >= 0 ) ? z : -z;

			float distFromCenter = absX + absZ;
			distFromCenter /= halfSize;	//1.0 should now be the maximum distance we want to see
			distFromCenter *= 1.5f;

			float y = centerHeight - ( horizonHeight * distFromCenter * distFromCenter );

			//cap height to a minimum
			if( y < minY )
				y = minY;

			pBuffer[ bufferIndex++ ].p = D3DXVECTOR3( x, y, z );
		}
	}

	//unlock the vertex buffer
	m_pVB->Unlock();

	//lock the index buffer
	WORD* pBuffer2 = NULL;
	bufferIndex = 0;
	if( FAILED( m_pIB->Lock( 0, NUM_FACES * 3 * sizeof( WORD ),
				(void**)&pBuffer2, 0 ) ) )
		return E_FAIL;

	//create the indices
	for( int row = 0; row < ( SKYPLANE_DIM - 1 ); ++row )
	{
		for( int column = 0; column < ( SKYPLANE_DIM - 1 ); ++column )
		{
			//create triangles for this quad
			WORD firstIndex = WORD( column + ( row * SKYPLANE_DIM ) );

			//triangle 1
			pBuffer2[ bufferIndex++ ] = firstIndex;
			pBuffer2[ bufferIndex++ ] = firstIndex + 1;
			pBuffer2[ bufferIndex++ ] = firstIndex + SKYPLANE_DIM;

			//triangle 2
			pBuffer2[ bufferIndex++ ] = firstIndex + SKYPLANE_DIM;
			pBuffer2[ bufferIndex++ ] = firstIndex + 1;
			pBuffer2[ bufferIndex++ ] = firstIndex + SKYPLANE_DIM + 1;
		}
	}

	//unlock the index buffer
	m_pIB->Unlock();

	OutputDebugString( "done\n" );

	return S_OK;
}