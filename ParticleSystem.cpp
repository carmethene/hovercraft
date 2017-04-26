//------------------------------------------------------------------------------
// File: ParticleSystem.cpp
// Desc: Representation of a particle system whose particles are under the
//		 influence of gravity, and whose particles have a constant lifetime
//
// Created: 25 January 2003 17:08:31
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <new>

#include "ParticleSystem.h"
#include "Resource.h"
#include "Scene.h"
#include "DXUtil.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: ParticleSystem()
// Desc: Constructor for the particle system class
//------------------------------------------------------------------------------
ParticleSystem::ParticleSystem( const int numParticles, const float particleLifetime )
{
	//initialise member variables
	m_numParticles		= numParticles;
	m_particleLifetime	= particleLifetime;
	m_initialPosition	= D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	m_initialVelocity	= D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	m_gravity			= D3DXVECTOR3( 0.0f, -1.0f, 0.0f );

	m_particlePositions		= NULL;
	m_particleVelocities	= NULL;
	m_particleAges			= NULL;

	m_pd3dDevice	= NULL;
	m_pVSDecl		= NULL;
	m_pVS			= NULL;

	//initialise the particles
	InitParticles();
}

//------------------------------------------------------------------------------
// Name: InitParticles()
// Desc: Creates storage space for particles and assigns an initial value 
//------------------------------------------------------------------------------
void ParticleSystem::InitParticles()
{
	//if storage has already been allocated, destroy it
	SAFE_DELETE_ARRAY( m_particlePositions );
	SAFE_DELETE_ARRAY( m_particleVelocities );
	SAFE_DELETE_ARRAY( m_particleAges );

	//allocate storage for the particles
	try{ m_particlePositions = new D3DXVECTOR3[ m_numParticles ]; }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		exit( 1 );
	}

	try{ m_particleVelocities = new D3DXVECTOR3[ m_numParticles ]; }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		exit( 1 );
	}

	try{ m_particleAges = new float[ m_numParticles ]; }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		exit( 1 );
	}

	//set initial particle parameters
	for( unsigned int particle = 0; particle < m_numParticles; ++particle )
	{
		m_particlePositions[ particle ]	 = m_initialPosition;
		m_particleVelocities[ particle ] = m_initialVelocity;
		m_particleAges[ particle ]		 = frand( rand() ) * m_particleLifetime;
	}
}

//------------------------------------------------------------------------------
// Name: ~ParticleSystem::
// Desc: Destructor for the particle system class
//------------------------------------------------------------------------------
ParticleSystem::~ParticleSystem()
{
	SAFE_DELETE_ARRAY( m_particlePositions );
	SAFE_DELETE_ARRAY( m_particleVelocities );
	SAFE_DELETE_ARRAY( m_particleAges );
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT ParticleSystem::InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice,
										   const bool dx9Shaders,
										   const bool twoSidedStencil )
{
	UNREFERENCED_PARAMETER( dx9Shaders );
	UNREFERENCED_PARAMETER( twoSidedStencil );

	//store a local copy of the direct3d device
	m_pd3dDevice = pd3dDevice;

	//create the vertex declaration
	D3DVERTEXELEMENT9 vsDecl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
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

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_PARTICLESYSTEM ),
												NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (particle system), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		return E_FAIL;
	}

	if( FAILED( m_pd3dDevice->CreateVertexShader( (const DWORD*)pCode->GetBufferPointer(),
												  &m_pVS ) ) )
		return E_FAIL;

	SAFE_RELEASE( pErrors );
	SAFE_RELEASE( pCode );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Sets up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT ParticleSystem::RestoreDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT ParticleSystem::InvalidateDeviceObjects()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT ParticleSystem::DeleteDeviceObjects()
{
	//remove reference to d3d device as it will disappear after this function is called
	m_pd3dDevice = NULL;

	//destroy the direct3d objects
	SAFE_RELEASE( m_pVSDecl );
	SAFE_RELEASE( m_pVS );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the object
//------------------------------------------------------------------------------
HRESULT ParticleSystem::Render( const Scene& scene ) const
{
	m_pd3dDevice->SetVertexDeclaration( m_pVSDecl );
	m_pd3dDevice->SetVertexShader( m_pVS );
	m_pd3dDevice->SetPixelShader( NULL );
	m_pd3dDevice->SetTexture( 0, NULL );
	m_pd3dDevice->SetTexture( 1, NULL );
	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

	//set vertex shader constants...
	//transform matrix
	D3DXMATRIX matViewProj = scene.GetCamera().GetViewProj();
	D3DXMATRIX matResult;
	D3DXMatrixTranspose( &matResult, &matViewProj );
	m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&matResult, 4 );

	m_pd3dDevice->DrawPrimitiveUP( D3DPT_POINTLIST, m_numParticles,
								   m_particlePositions, sizeof(D3DXVECTOR3) );

	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: UpdateParticles()
// Desc: Updates the particle positions and lifetimes
//------------------------------------------------------------------------------
HRESULT ParticleSystem::UpdateParticles( const float timeStep )
{
	//calculate change in velocity
	const D3DXVECTOR3 velocityDelta = m_gravity * timeStep;

	//track particles created this frame
	unsigned int particlesCreated = 0;

	unsigned int maxCreation = unsigned int( timeStep / 
								( m_particleLifetime / m_numParticles ) );

	//for each particle
	for( unsigned int particle = 0; particle < m_numParticles; ++particle )
	{
		//see if it is time to kill the particle
		m_particleAges[ particle ]	 += timeStep;
		if( m_particleAges[ particle ] >= m_particleLifetime )
		{
			//check that we haven't already created too many particles during this update
			if( particlesCreated > maxCreation )
				continue;
			else
				particlesCreated++;

			//particle is dead, kill it and create a new one
			m_particleAges[ particle ]			= 0.0f;
			m_particlePositions[ particle ]		= m_initialPosition;
			m_particleVelocities[ particle ]	= m_initialVelocity;

			//if we are moving slowly, do not produce particles
			if( m_initialSpeed < 17.0f )
			{
				//simply hide the particles
				m_particlePositions[ particle ].y = -1000.0f;
			}

			//jitter the particle position and velocity to create a particle
			//cloud instead of a thin stream
			const float maxJitterAngle	= 2.0f;

			//jitter velocity
			const float jitterAngle = float( gaussianRand( 0.0f, maxJitterAngle ) );
			D3DXMATRIX matJitter;
			D3DXMatrixRotationY( &matJitter, jitterAngle );
			D3DXVECTOR4 temp;
			D3DXVec3Transform( &temp, &m_particleVelocities[ particle ], &matJitter );
			m_particleVelocities[ particle ] = D3DXVECTOR3( temp.x, temp.y, temp.z );
			m_particleVelocities[ particle ].y += frand( rand() );

			//jitter position
			m_particlePositions[ particle ] -= m_initialVelocity;
			m_particlePositions[ particle ] += m_particleVelocities[ particle ];
		}
		else
		{
			//particle is alive, update velocity and position
			m_particleVelocities[ particle ] += velocityDelta;
			
			m_particlePositions[ particle ] +=
				m_particleVelocities[ particle ] * timeStep;
		}
	}

	return S_OK;
}
