//------------------------------------------------------------------------------
// File: Backdrop.cpp
// Desc: Quad with a background texture
//
// Created: 05 January 2003 16:23:58
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "Backdrop.h"
#include "Resource.h"
#include "Scene.h"

#include "DXUtil.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: struct BackdropVertex
// Desc: A single vertex in the backdrop
//------------------------------------------------------------------------------
struct BackdropVertex
{
	FLOAT x, y, z;	//position
	FLOAT tu, tv;	//texture coordinates
};

//------------------------------------------------------------------------------
// Name: Backdrop()
// Desc: Constructor for the Backdrop object
//------------------------------------------------------------------------------
Backdrop::Backdrop()
{
	//initialise member variables
	m_pd3dDevice = NULL;
	m_pVB		 = NULL;
	m_pIB		 = NULL;

	m_pVS		 = NULL;
	m_pVSDecl	 = NULL;

	m_pBackdropTexture = NULL;
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT Backdrop::InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice,
									 const bool dx9Shaders,
									 const bool twoSidedStencil )
{
	UNREFERENCED_PARAMETER( dx9Shaders );
	UNREFERENCED_PARAMETER( twoSidedStencil );

	//store a local copy of the direct3d device
	m_pd3dDevice = pd3dDevice;

	//create the vertex buffer
	OutputDebugString( "Creating backdrop buffers..." );

	const int VB_SIZE = 4 * sizeof( BackdropVertex );
	if( FAILED( m_pd3dDevice->CreateVertexBuffer( VB_SIZE, D3DUSAGE_WRITEONLY, 0,
												  D3DPOOL_MANAGED, &m_pVB, NULL ) ) )
		return E_FAIL;

	BackdropVertex vertices[] =
	{
		{ -250.0f, -1.0f, -250.0f, 0.0f, 0.0f },
		{ -250.0f, -1.0f, 250.0f, 0.0f, 1.0f },
		{ 250.0f, -1.0f, 250.0f, 1.0f, 1.0f },
		{ 250.0f, -1.0f, -250.0f, 1.0f, 0.0f },
	};

	BackdropVertex* pBuffer = NULL;
	if( FAILED( m_pVB->Lock( 0, sizeof( vertices ), (void**)&pBuffer, 0 ) ) )
		return E_FAIL;
	memcpy( pBuffer, vertices, sizeof( vertices ) );
	m_pVB->Unlock();

	//create the cloud texture
	if( FAILED( D3DXCreateTextureFromResource( m_pd3dDevice, NULL,
				MAKEINTRESOURCE( IDD_TEX_BACKDROP ), &m_pBackdropTexture ) ) )
		return E_FAIL;

	OutputDebugString( "done\n" );

	OutputDebugString( "Creating backdrop shaders..." );

	//define the vertex shader
	D3DVERTEXELEMENT9 vsDecl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
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

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_BACKDROP ),
												NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (backdrop), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		return E_FAIL;
	}

    if( FAILED( m_pd3dDevice->CreateVertexShader( (const DWORD*)pCode->GetBufferPointer(),
												  &m_pVS ) ) )
		return E_FAIL;

	SAFE_RELEASE( pErrors );
	SAFE_RELEASE( pCode );

	OutputDebugString( "done\n" );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Sets up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT Backdrop::RestoreDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT Backdrop::InvalidateDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT Backdrop::DeleteDeviceObjects()
{
	//destroy the vertex buffer
	SAFE_RELEASE( m_pVB );

	//destroy the noise texture
	SAFE_RELEASE( m_pBackdropTexture );

	//destroy the shaders
	SAFE_RELEASE( m_pVS );
	SAFE_RELEASE( m_pVSDecl );

	m_pd3dDevice = NULL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the object
//------------------------------------------------------------------------------
HRESULT Backdrop::Render( const Scene& scene )
{
	//set vertex shader constants...
	//transform matrix
	D3DXMATRIX matViewProj = scene.GetCamera().GetViewProj();	

	//camera should be at the center of the backdrop
	D3DXMATRIX matWorld;
	const D3DXVECTOR3& vCamPosition = scene.GetCamera().GetPosition();
	D3DXMatrixTranslation( &matWorld, vCamPosition[0], vCamPosition[1], vCamPosition[2] );

	//set the transform matrix
	D3DXMATRIX matResult;
	D3DXMatrixMultiplyTranspose( &matResult, &matWorld, &matViewProj );
	m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&matResult, 4 );

	//set device parameters
	m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof( BackdropVertex ) );
	m_pd3dDevice->SetVertexDeclaration( m_pVSDecl );
	m_pd3dDevice->SetVertexShader( m_pVS );
	m_pd3dDevice->SetPixelShader( NULL );
	m_pd3dDevice->SetTexture( 0, m_pBackdropTexture );

	//draw the object
	m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 );

	return S_OK;
}
