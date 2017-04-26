//------------------------------------------------------------------------------
// File: Vehicle.cpp
// Desc: Hovercraft vehicle and its physics simulation
//
// Created: 06 January 2003 18:18:47
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "Scene.h"
#include "Vehicle.h"
#include "Resource.h"

#include "DXUtil.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------
const float Vehicle::SIZE_X = 6.0f;
const float Vehicle::SIZE_Y = 1.0f;
const float Vehicle::SIZE_Z = 6.0f;

//------------------------------------------------------------------------------
// Name: struct VehicleVertex
// Desc: A single vertex in the vehicle mesh
//------------------------------------------------------------------------------
struct VehicleVertex
{
	D3DXVECTOR3 p;
	D3DXVECTOR3 n;
};

//------------------------------------------------------------------------------
// Name: Vehicle()
// Desc: Constructor for the vehicle object
//------------------------------------------------------------------------------
Vehicle::Vehicle()
{
	//initialise member variables
	m_pd3dDevice	= NULL;
	m_pMesh			= NULL;
	m_numMaterials	= NULL;
	m_pMaterials	= NULL;
	m_pVSAmbient	= NULL;
	m_pVSDiffuse	= NULL;
	m_pVSDecl		= NULL;
	m_pPS			= NULL;
	m_isOnGround	= false;

	//initialise physics constants
	m_mass				= 150.0f;
	m_vPosition			= D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f );
	m_vLinearVelocity	= D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f );
	D3DXMatrixIdentity( &m_matRotation );
	m_vAngularMomentum	= D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f );

	//calculate inertia tensor
	D3DXMatrixIdentity( &m_matInverseTensor );
	const float m = m_mass / 12.0f;
	const float x = SIZE_X * SIZE_X;
	const float y = SIZE_Y * SIZE_Y;
	const float z = SIZE_Z * SIZE_Z;
	m_matInverseTensor( 0, 0 ) = m * ( y + z );	//x-axis
	m_matInverseTensor( 1, 1 ) = m * ( x + z );	//y-axis
	m_matInverseTensor( 2, 2 ) = m * ( x + y );	//z-axis
	D3DXMatrixInverse( &m_matInverseTensor, NULL, &m_matInverseTensor );

	//calculate auxiliary quanitites...
	//world-space inertia tensor
	D3DXMATRIX matRotationTranspose;
	D3DXMatrixTranspose( &matRotationTranspose, &m_matRotation );
	D3DXMatrixMultiply( &m_matWorldTensor, &m_matRotation, &m_matInverseTensor );
	D3DXMatrixMultiply( &m_matWorldTensor, &m_matWorldTensor, &matRotationTranspose );

	//angular velocity
	D3DXVec4Transform( &m_vAngularVelocity, &m_vAngularMomentum, &m_matWorldTensor );

	//create collision grid
	const float halfX = SIZE_X / 2.0f;
	const float halfY = SIZE_Y / 2.0f;
	const float halfZ = SIZE_Z / 2.0f;
	const float stepX = SIZE_X / ( POINTS_PER_EDGE - 1 );
	const float stepZ = SIZE_Z / ( POINTS_PER_EDGE - 1 );

	for( int x = 0; x < POINTS_PER_EDGE; ++x )
	{
		for( int z = 0; z < POINTS_PER_EDGE; ++z )
		{
			m_collisionVectors[ x ][ z ] = D3DXVECTOR3( halfX - ( x * stepX ),
														halfY,
														halfZ - ( z * stepZ ) );

            m_collisionPoints[ x ][ z ] = D3DXVECTOR4( ( x * stepX ) - halfX,
													   - halfY,
													   ( z * stepZ ) - halfZ,
													   1.0f );
		}
	}
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT Vehicle::InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice,
								    const bool dx9Shaders,
									const bool twoSidedStencil )
{
	//store a local copy of the direct3d device
	m_pd3dDevice = pd3dDevice;

	OutputDebugString( "Creating vehicle shaders..." );

	//create the vertex declaration
	D3DVERTEXELEMENT9 vsDecl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
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

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_STATIC_AMBIENT ),
												NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (static ambient), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		return E_FAIL;
	}

	if( FAILED( m_pd3dDevice->CreateVertexShader( (const DWORD*)pCode->GetBufferPointer(),
												  &m_pVSAmbient ) ) )
		return E_FAIL;

	SAFE_RELEASE( pErrors );
	SAFE_RELEASE( pCode );

	if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_VS_STATIC_DIFFUSE ),
												NULL, NULL, flags, &pCode, &pErrors ) ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (static diffuse), errors:\n" );
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
		if( FAILED( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDD_PS_STATIC ),
													NULL, NULL, flags, &pCode, &pErrors ) ) )
		{
			OutputDebugString( "Failed to assemble pixel shader (static), errors:\n" );
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

	//initialise the shadow volume
	if( FAILED( m_shadowVolume.InitDeviceObjects( pd3dDevice, dx9Shaders, twoSidedStencil ) ) )
		return E_FAIL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Sets up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT Vehicle::RestoreDeviceObjects()
{
	OutputDebugString( "Creating vehicle buffers..." );

	//create the mesh
	LPD3DXBUFFER pD3DXMtrlBuffer;
    if( FAILED( D3DXLoadMeshFromXResource( NULL, MAKEINTRESOURCE( IDD_X_HOVERCRAFT ),
										   "X", D3DXMESH_SYSTEMMEM, m_pd3dDevice,
										   NULL, &pD3DXMtrlBuffer, NULL, &m_numMaterials,
										   &m_pMesh ) ) )
		return E_FAIL;

	//load materials
	m_pMaterials = new D3DXVECTOR4[ m_numMaterials ];
	D3DXMATERIAL* pMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
	for( DWORD i = 0; i < m_numMaterials; ++i )
	{
		D3DMATERIAL9& mat = pMaterials[ i ].MatD3D;
		m_pMaterials[ i ] = D3DXVECTOR4( mat.Diffuse.r, mat.Diffuse.g,
										 mat.Diffuse.b, mat.Diffuse.a );
	}
	SAFE_RELEASE( pD3DXMtrlBuffer );

	OutputDebugString( "done\n" );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT Vehicle::InvalidateDeviceObjects()
{
	//destroy the vehicle mesh
	SAFE_DELETE_ARRAY( m_pMaterials );
	SAFE_RELEASE( m_pMesh );
	m_numMaterials = 0;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT Vehicle::DeleteDeviceObjects()
{
	//destroy the shadow volume
	m_shadowVolume.DeleteDeviceObjects();

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
HRESULT Vehicle::Render( const Scene& scene, const bool useLight,
						 const bool renderShadowVolume ) const
{
	//set vertex shader constants...
	//transform matrix
	D3DXMATRIX matViewProj = scene.GetCamera().GetViewProj();
	D3DXMATRIX matTranslate, matScale;
	D3DXMatrixTranslation( &matTranslate, m_vPosition[0], m_vPosition[1], m_vPosition[2] );
	D3DXMatrixScaling( &matScale, 0.2f, 0.2f, 0.2f );

	D3DXMATRIX matWorld;
	D3DXMatrixMultiply( &matWorld, &matScale, &matTranslate );
	D3DXMatrixMultiply( &matWorld, &m_matRotation, &matWorld );

	D3DXMATRIX matResult;
	D3DXMatrixMultiplyTranspose( &matResult, &matWorld, &matViewProj );
	m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&matResult, 4 );

	//transposed inverse-transpose world matrix for transforming normals
	D3DXMatrixInverse( &matResult, NULL, &matWorld );
	m_pd3dDevice->SetVertexShaderConstantF( 4, (float*)&matResult, 4 );

	//which lighting mode are we using?
	if( useLight )
	{
		//diffuse
		D3DXVECTOR4 vLightDirection = scene.GetLight( 0 ).GetPosition();
		m_pd3dDevice->SetVertexShaderConstantF( 9, (float*)&vLightDirection, 1 );
		D3DXVECTOR4 vLightColour	= scene.GetLight( 0 ).GetColour();
		m_pd3dDevice->SetVertexShaderConstantF( 10, (float*)&vLightColour, 1 );

		m_pd3dDevice->SetVertexShader( m_pVSDiffuse );
	}
	else
	{
		//ambient
		D3DXVECTOR4 vAmbientColour = scene.GetAmbientLight();
		m_pd3dDevice->SetVertexShaderConstantF( 9, (float*)&vAmbientColour, 1 );

        m_pd3dDevice->SetVertexShader( m_pVSAmbient );
	}

	//set device parameters
	m_pd3dDevice->SetVertexDeclaration( m_pVSDecl );
	m_pd3dDevice->SetPixelShader( m_pPS );

	//draw the object
	for( DWORD i = 0; i < m_numMaterials; ++i )
	{
		m_pd3dDevice->SetVertexShaderConstantF( 8, (float*)&m_pMaterials[ i ], 1 );
		m_pMesh->DrawSubset( i );
	}

	//draw the shadow volume
	if( renderShadowVolume )
        m_shadowVolume.Render();

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: UpdateShadowVolume()
// Desc: Rebuilds the shadow volume for the current position
//------------------------------------------------------------------------------
HRESULT Vehicle::UpdateShadowVolume( const Scene& scene, const bool showVolumes )
{
	m_shadowVolume.ShowVolumes( showVolumes );

	//transform light into object space
	D3DXMATRIX m;
	D3DXMatrixInverse( &m, NULL, &m_matRotation );

    D3DXVECTOR3 vL = scene.GetLight( 0 ).GetPosition();
	D3DXVECTOR3 vLight;
	vLight.x = vL.x*m._11 + vL.y*m._21 + vL.z*m._31 + m._41;
	vLight.y = vL.x*m._12 + vL.y*m._22 + vL.z*m._32 + m._42;
	vLight.z = vL.x*m._13 + vL.y*m._23 + vL.z*m._33 + m._43;

	m_shadowVolume.Reset();
	m_shadowVolume.BuildFromMesh( m_pMesh, vLight );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DoPhysics()
// Desc: Runs the physics simulation for the vehicle by one frame
//------------------------------------------------------------------------------
void Vehicle::DoPhysics( const float timeInterval, const Terrain* pTerrain,
						 const bool forwardThrust, const bool reverseThrust,
						 const bool leftThrust, const bool rightThrust )
{
	//simulation constants
	const static float GRAVITY = 100.0f;
	const static float LINEAR_THRUST = 40000.0f;
	const static float ANGULAR_THRUST = 500.0f;
	const static float LINEAR_AR = 30.0f;
	const static float ANGULAR_AR = 1000.0f;
	const static float HOVER_HEIGHT = 1.0f;
	const static float SUPPORT_HEIGHT = HOVER_HEIGHT - 0.5f;

	//vectors needed for physics calculations, don't change these...
	const static D3DXVECTOR4 vGravity( 0.0f, -GRAVITY, 0.0f, 1.0f );

	const static D3DXVECTOR4 vLinearThrustBody( 0.0f, 0.0f, LINEAR_THRUST, 1.0f );
	D3DXVECTOR4 vLinearThrust;
	D3DXVec4Transform( &vLinearThrust, &vLinearThrustBody, &m_matRotation );

	const static D3DXVECTOR4 vAngularThrustBody( ANGULAR_THRUST, 0.0f, 0.0f, 1.0f );
	D3DXVECTOR4 vAngularThrust;
	D3DXVec4Transform( &vAngularThrust, &vAngularThrustBody, &m_matRotation );
	const static D3DXVECTOR4 vAngularTorque( 0.0f, 2.0f * ANGULAR_THRUST * SIZE_Z, 0.0f, 1.0f );

	const D3DXVECTOR4
		vLinearVelocitySq(  m_vLinearVelocity[ 0 ] * fabsf( m_vLinearVelocity[ 0 ] ),
							m_vLinearVelocity[ 1 ] * fabsf( m_vLinearVelocity[ 1 ] ),
							m_vLinearVelocity[ 2 ] * fabsf( m_vLinearVelocity[ 2 ] ), 1.0f );

	const static D3DXVECTOR3 vUp( 0.0f, 1.0f, 0.0f );
	D3DXVECTOR4 vNormal;
	D3DXVec3Transform( &vNormal, &vUp, &m_matRotation );
	D3DXVECTOR3 vNormal3( vNormal[ 0 ], vNormal[ 1 ], vNormal[ 2 ] );

	//calculate forces...
	D3DXVECTOR4 vForce( 0.0f, 0.0f, 0.0f, 1.0f );
	D3DXVECTOR4 vTorque( 0.0f, 0.0f, 0.0f, 0.0f );

	//terrain collision forces for each point in the collision grid
	float moveHeight = 0.0f;
	bool haveAppliedUpthrust = false;
	bool nearTerrain = false;
	m_isOnGround = false;

	for( int x = 0; x < POINTS_PER_EDGE; ++x )
	{
		for( int z = 0; z < POINTS_PER_EDGE; ++z )
		{
			//translate this point to world space
			D3DXVECTOR4 vPoint = m_collisionPoints[ x ][ z ];
			D3DXVec4Transform( &vPoint, &vPoint, &m_matRotation );
			vPoint += m_vPosition;

			//find the heightmap value at this point
			float terrainHeight = pTerrain->GetHeightMapPoint( vPoint[ 0 ], vPoint[ 2 ] );
			float terrainDistance = vPoint[ 1 ] - terrainHeight;

			if( terrainDistance < 1.0f )
			{
				nearTerrain = true;
			}
			
			//do physics on this point
			if( terrainDistance < HOVER_HEIGHT )
			{
				m_isOnGround = true;

				//within hover distance, add a hover force to counteract gravity
				const static int NUM_POINTS = POINTS_PER_EDGE * POINTS_PER_EDGE;
				D3DXVECTOR4 vTemp = -vGravity * m_mass / float( POINTS_PER_EDGE );
                D3DXVECTOR4 vPointForce = vNormal * D3DXVec4Dot( &vTemp, &vNormal );

				//add linear force
				if( ! haveAppliedUpthrust )
				{
                    vForce += vPointForce;
					haveAppliedUpthrust = true;
				}

				//add an angular displacement
				D3DXVECTOR3 vPointForce3( vPointForce[ 0 ], vPointForce[ 1 ],
										  vPointForce[ 2 ] );
				D3DXVECTOR3 vPointDisplacement3;
				D3DXVec3Cross( &vPointDisplacement3, &m_collisionVectors[ x ][ z ],
							   &vPointForce3 );
				D3DXVECTOR4 vPointDisplacement( vPointDisplacement3[ 0 ],
												vPointDisplacement3[ 1 ],
												vPointDisplacement3[ 2 ], 0.0f );
				
				//scale displacement
				vPointDisplacement *= timeInterval;
				vPointDisplacement *= 0.0002f;

				D3DXMATRIX matTemp;
				memset( &matTemp, 0, sizeof( matTemp ) );
				matTemp( 0, 1 ) = -vPointDisplacement[ 2 ];
				matTemp( 0, 2 ) =  vPointDisplacement[ 1 ];
				matTemp( 1, 0 ) =  vPointDisplacement[ 2 ];
				matTemp( 1, 2 ) = -vPointDisplacement[ 0 ];
				matTemp( 2, 0 ) = -vPointDisplacement[ 1 ];
				matTemp( 2, 1 ) =  vPointDisplacement[ 0 ];
				m_matRotation += matTemp * m_matRotation;
			}

			//see if the supports are in contact with the ground
			if( terrainDistance < SUPPORT_HEIGHT )
			{
				//kill velocity along the surface normal
				D3DXVECTOR3 vVelocity3( - m_vLinearVelocity[ 0 ], - m_vLinearVelocity[ 1 ], - m_vLinearVelocity[ 2 ] );
				D3DXVECTOR3 vKillVelocity3 = vNormal3 * D3DXVec3Dot( &vVelocity3, &vNormal3 );
				D3DXVECTOR4 vKillVelocity( vKillVelocity3[ 0 ], vKillVelocity3[ 1 ], vKillVelocity3[ 2 ], 0.0f );
				m_vLinearVelocity += vKillVelocity;
			}

			if( terrainDistance < 0.0f )
			{
				//collision, move object to compensate
				if( terrainDistance < moveHeight )
					moveHeight = terrainDistance;
			}
		}
	}
	//make sure we don't penetrate the terrain
	m_vPosition -= D3DXVECTOR4( 0.0f, moveHeight, 0.0f, 0.0f );

	//linear force
	vForce += vGravity * m_mass;
	if( !nearTerrain ) vLinearThrust[ 1 ] = 0.0f;
	if( forwardThrust ) vForce += vLinearThrust;	//engine thrust
	if( reverseThrust ) vForce -= vLinearThrust;
	if( leftThrust ) vForce -= vAngularThrust;		//sideways thrust caused by turning
	if( rightThrust ) vForce += vAngularThrust;
	vForce += ( -vLinearVelocitySq ) * LINEAR_AR;	//air resistance
	
	//torque
	if( leftThrust ) vTorque += vAngularTorque;
	if( rightThrust ) vTorque -= vAngularTorque;
	vTorque += ( -m_vAngularVelocity ) * ANGULAR_AR;	//air resistance

	//integrate quantities...
	//linear velocity
	D3DXVECTOR4 vTemp = m_vLinearVelocity * timeInterval;
	m_vPosition += vTemp;
	m_vPosition[ 3 ] = 1.0f;

	//linear acceleration
	vTemp = vForce / m_mass;
	vTemp *= timeInterval;
    m_vLinearVelocity += vTemp;
	m_vLinearVelocity[ 3 ] = 1.0f;

	//angular velocity
	D3DXMATRIX matTemp;
	memset( &matTemp, 0, sizeof( matTemp ) );
	matTemp( 0, 1 ) = -m_vAngularVelocity[ 2 ];
	matTemp( 0, 2 ) = m_vAngularVelocity[ 1 ];
	matTemp( 1, 0 ) = m_vAngularVelocity[ 2 ];
	matTemp( 1, 2 ) = -m_vAngularVelocity[ 0 ];
	matTemp( 2, 0 ) = -m_vAngularVelocity[ 1 ];
	matTemp( 2, 1 ) = m_vAngularVelocity[ 0 ];
	matTemp *= timeInterval;
	m_matRotation += matTemp * m_matRotation;

	//torque
	vTemp = vTorque * timeInterval;
	m_vAngularMomentum += vTemp;
	m_vAngularMomentum[ 3 ] = 1.0f;

	//reorthogonalise the rotation matrix
	D3DXVECTOR3 row0( m_matRotation( 0, 0 ), m_matRotation( 0, 1 ), m_matRotation( 0, 2 ) );
	D3DXVECTOR3 row1( m_matRotation( 1, 0 ), m_matRotation( 1, 1 ), m_matRotation( 1, 2 ) );
	D3DXVECTOR3 row2( m_matRotation( 2, 0 ), m_matRotation( 2, 1 ), m_matRotation( 2, 2 ) );

	D3DXVec3Normalize( &row0, &row0 );
	D3DXVec3Cross( &row2, &row0, &row1 );
	D3DXVec3Normalize( &row2, &row2 );
	D3DXVec3Cross( &row1, &row2, &row0 );
	D3DXVec3Normalize( &row1, &row1 );

	m_matRotation( 0, 0 ) = row0[ 0 ];
	m_matRotation( 0, 1 ) = row0[ 1 ];
	m_matRotation( 0, 2 ) = row0[ 2 ];
	m_matRotation( 1, 0 ) = row1[ 0 ];
	m_matRotation( 1, 1 ) = row1[ 1 ];
	m_matRotation( 1, 2 ) = row1[ 2 ];
	m_matRotation( 2, 0 ) = row2[ 0 ];
	m_matRotation( 2, 1 ) = row2[ 1 ];
	m_matRotation( 2, 2 ) = row2[ 2 ];

	//calculate auxiliary quanitites...
	//world-space inertia tensor
	D3DXMATRIX matRotationTranspose;
	D3DXMatrixTranspose( &matRotationTranspose, &m_matRotation );
	D3DXMatrixMultiply( &m_matWorldTensor, &m_matRotation, &m_matInverseTensor );
	D3DXMatrixMultiply( &m_matWorldTensor, &m_matWorldTensor, &matRotationTranspose );

	//angular velocity
	D3DXVec4Transform( &m_vAngularVelocity, &m_vAngularMomentum, &m_matWorldTensor );

	//cap position to keep vehicle on the terrain
	float maxDisplacement = pTerrain->GetTerrainSize() - 5.0f;
	if( m_vPosition[ 0 ] < 5.0f ) m_vPosition[ 0 ] = 5.0f;
	if( m_vPosition[ 0 ] > maxDisplacement ) m_vPosition[ 0 ] = maxDisplacement;
	if( m_vPosition[ 2 ] < 5.0f ) m_vPosition[ 2 ] = 5.0f;
	if( m_vPosition[ 2 ] > maxDisplacement ) m_vPosition[ 2 ] = maxDisplacement;
}
