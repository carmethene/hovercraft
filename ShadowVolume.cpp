//------------------------------------------------------------------------------
// File: ShadowVolume.cpp
// Desc: A representation of a shadow volume along with methods for generating
//		 and rendering it
//
// Created: 09 January 2003 13:58:16
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <new>

#include "ShadowVolume.h"
#include "Resource.h"

#include "DXUtil.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: ShadowVolume()
// Desc: Constructor for the shadow volume object
//------------------------------------------------------------------------------
ShadowVolume::ShadowVolume()
{
	//initialise member pointers
	m_pd3dDevice	= NULL;
	m_pVS			= NULL;
	m_pVSDecl		= NULL;
	m_numVertices	= 0;

	m_showVolumes = false;
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT ShadowVolume::InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice,
										 const bool dx9Shaders,
										 const bool twoSidedStencil )
{
	UNREFERENCED_PARAMETER( dx9Shaders );

	//store the direct3d device
	m_pd3dDevice = pd3dDevice;

	//store stencil buffer type
	m_twoSidedStencil = twoSidedStencil;

	OutputDebugString( "Creating shadow volume shaders..." );

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

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_SHADOWVOLUME ),
												NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (shadow volume), errors:\n" );
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
// Name: DeleteDeviceObjects()
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT ShadowVolume::DeleteDeviceObjects()
{
	//destroy the shaders
	SAFE_RELEASE( m_pVS );
	SAFE_RELEASE( m_pVSDecl );

	m_pd3dDevice = NULL;

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the shadow volume
//-----------------------------------------------------------------------------
HRESULT ShadowVolume::Render() const
{
	//set device states
	m_pd3dDevice->SetVertexDeclaration( m_pVSDecl );
	m_pd3dDevice->SetVertexShader( m_pVS );
	m_pd3dDevice->SetPixelShader( NULL );

	//set render states required for colour and stencil buffers
	m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

	if( m_showVolumes )
	{
		m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, m_numVertices/3,
									m_pVertices, sizeof(D3DXVECTOR3) );
	}

	//disable z-buffer writes (note: z-testing still occurs), and enable the stencil-buffer
	m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
	m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, TRUE );

	//dont bother with interpolating color
	m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );

	//set up stencil compare fuction, reference value, and masks.
	//stencil test passes if ((ref & mask) cmpfn (stencil & mask)) is true.
	//note: since we set up the stencil-test to always pass, the STENCILFAIL
	//renderstate is really not needed.
	m_pd3dDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
	m_pd3dDevice->SetRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );
	m_pd3dDevice->SetRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );

	//if ztest passes, inc/decrement stencil buffer value
	m_pd3dDevice->SetRenderState( D3DRS_STENCILREF, 0x1 );
	m_pd3dDevice->SetRenderState( D3DRS_STENCILMASK, 0xffffffff );
	m_pd3dDevice->SetRenderState( D3DRS_STENCILWRITEMASK, 0xffffffff );
	m_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INCR );

	// Make sure that no pixels get drawn to the frame buffer
	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
	m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

	if( m_twoSidedStencil )
	{
		//with 2-sided stencil, we can avoid rendering twice
		m_pd3dDevice->SetRenderState( D3DRS_TWOSIDEDSTENCILMODE, TRUE );
		m_pd3dDevice->SetRenderState( D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS );
		m_pd3dDevice->SetRenderState( D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP );
		m_pd3dDevice->SetRenderState( D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP );
		m_pd3dDevice->SetRenderState( D3DRS_CCW_STENCILPASS, D3DSTENCILOP_DECR );
		m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

		//draw both sides of shadow volume in stencil/z only
		m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, m_numVertices/3,
									m_pVertices, sizeof(D3DXVECTOR3) );

		m_pd3dDevice->SetRenderState( D3DRS_TWOSIDEDSTENCILMODE, FALSE );
	}
	else
	{
		//draw front-side of shadow volume in stencil/z only
		m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
		m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, m_numVertices/3,
									m_pVertices, sizeof(D3DXVECTOR3) );

		//decrement stencil buffer value
		m_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_DECR );

		//draw back-side of shadow volume in stencil/z only
		m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
		m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, m_numVertices/3,
									m_pVertices, sizeof(D3DXVECTOR3) );
	}

	//restore render states
	m_pd3dDevice->SetRenderState( D3DRS_TWOSIDEDSTENCILMODE, FALSE );
	m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: BuildFromMesh()
// Desc: Takes a mesh as input, and uses it to build a shadowvolume. The
//       technique used considers each triangle of the mesh, and adds it's
//       edges to a temporary list. The edge list is maintained, such that
//       only silohuette edges are kept. Finally, the silohuette edges are
//       extruded to make the shadow volume vertex list.
//-----------------------------------------------------------------------------
HRESULT ShadowVolume::BuildFromMesh( LPD3DXMESH pMesh, D3DXVECTOR3 vLight )
{
	//note: the MESHVERTEX format depends on the FVF of the mesh
	struct MESHVERTEX
	{
		D3DXVECTOR3 p, n;
	};

	vLight = -vLight;

	MESHVERTEX* pVertices;
	WORD*       pIndices;

	//lock the geometry buffers
	pMesh->LockVertexBuffer( 0, (LPVOID*)&pVertices );
	pMesh->LockIndexBuffer( 0, (LPVOID*)&pIndices );
	DWORD numFaces = pMesh->GetNumFaces();

	//allocate a temporary edge list
	WORD* pEdges = NULL;
	try{ pEdges = new WORD[ numFaces * 6 ]; }
	catch( std::bad_alloc& error )
	{
		pMesh->UnlockVertexBuffer();
		pMesh->UnlockIndexBuffer();
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}
	DWORD numEdges = 0;

	//for each face
	for( DWORD i = 0; i < numFaces; ++i )
	{
		WORD wFace0 = pIndices[ 3*i + 0 ];
		WORD wFace1 = pIndices[ 3*i + 1 ];
		WORD wFace2 = pIndices[ 3*i + 2 ];

		D3DXVECTOR3 v0 = pVertices[ wFace0 ].p;
		D3DXVECTOR3 v1 = pVertices[ wFace1 ].p;
		D3DXVECTOR3 v2 = pVertices[ wFace2 ].p;

		//transform vertices or transform light?
		D3DXVECTOR3 vCross1(v2-v1);
		D3DXVECTOR3 vCross2(v1-v0);
		D3DXVECTOR3 vNormal;
		D3DXVec3Cross( &vNormal, &vCross1, &vCross2 );

		if( D3DXVec3Dot( &vNormal, &vLight ) >= 0.0f )
		{
			//add edges to the list
			pEdges[ 2*numEdges ] = wFace0;
			pEdges[ 2*numEdges + 1 ] = wFace1;
			numEdges++;
			pEdges[ 2*numEdges ] = wFace1;
			pEdges[ 2*numEdges + 1 ] = wFace2;
			numEdges++;
			pEdges[ 2*numEdges ] = wFace2;
			pEdges[ 2*numEdges + 1 ] = wFace0;
			numEdges++;
		}
	}

	for( DWORD i = 0;  i < numEdges; ++i )
	{
		D3DXVECTOR3 v1 = pVertices[ pEdges[ 2*i + 0 ] ].p;
		D3DXVECTOR3 v2 = pVertices[ pEdges[ 2*i + 1 ] ].p;
		D3DXVECTOR3 v3 = v1 - vLight * 100;
		D3DXVECTOR3 v4 = v2 - vLight * 100;

		//add a quad (two triangles) to the vertex list
		m_pVertices[ m_numVertices++ ] = v1;
		m_pVertices[ m_numVertices++ ] = v2;
		m_pVertices[ m_numVertices++ ] = v3;

		m_pVertices[ m_numVertices++ ] = v2;
		m_pVertices[ m_numVertices++ ] = v4;
		m_pVertices[ m_numVertices++ ] = v3;
	}
	//delete the temporary edge list
	delete[] pEdges;

	//unlock the geometry buffers
	pMesh->UnlockVertexBuffer();
	pMesh->UnlockIndexBuffer();

    return S_OK;
}
