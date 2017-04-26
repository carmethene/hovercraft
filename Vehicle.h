//------------------------------------------------------------------------------
// File: Vehicle.h
// Desc: Hovercraft vehicle with physics simulation
//
// Created: 06 January 2003 18:18:16
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_VEHICLE_H
#define INCLUSIONGUARD_VEHICLE_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "ShadowVolume.h"
#include "Terrain.h"


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class Scene;

//------------------------------------------------------------------------------
// Name: class Vehicle
// Desc: The hovercraft object
//------------------------------------------------------------------------------
class Vehicle
{
public:
	Vehicle();

	HRESULT InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice, const bool dx9Shaders,
							   const bool twoSidedStencil );
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT DeleteDeviceObjects();

	HRESULT Render( const Scene& scene, const bool useLight,
					const bool renderShadowVolume ) const;

	HRESULT UpdateShadowVolume( const Scene& scene, const bool showVolumes );

	void DoPhysics( const float timeInterval, const Terrain* pTerrain,
					const bool forwardThrust, const bool reverseThrust,
					const bool leftThrust, const bool rightThrust );

	inline void SetPosition( const D3DXVECTOR3 vPos )
	{
		m_vPosition = D3DXVECTOR4( vPos[ 0 ], vPos[ 1 ], vPos[ 2 ], 1.0f );
	}

	inline D3DXVECTOR3 GetPosition() const
	{
		return D3DXVECTOR3( m_vPosition[ 0 ], m_vPosition[ 1 ], m_vPosition[ 2 ] );
	}

	inline D3DXVECTOR3 GetVelocity() const
	{
		return D3DXVECTOR3( m_vLinearVelocity[ 0 ],
							m_vLinearVelocity[ 1 ],
							m_vLinearVelocity[ 2 ] );
	}

	inline D3DXVECTOR3 GetDirection() const
	{
		D3DXVECTOR4 vTemp( 0.0f, 0.0f, 1.0f, 1.0f );
		D3DXVec4Transform( &vTemp, &vTemp, &m_matRotation );
		D3DXVECTOR3 vDirection( vTemp[ 0 ], vTemp[ 1 ], vTemp[ 2 ] );
		D3DXVec3Normalize( &vDirection, &vDirection );
		return vDirection;
	}

	inline bool IsOnGround() { return m_isOnGround; }

private:
	//direct3d objects
	LPDIRECT3DDEVICE9		m_pd3dDevice;
	LPD3DXMESH				m_pMesh;
	DWORD					m_numMaterials;
	D3DXVECTOR4*			m_pMaterials;
	LPDIRECT3DVERTEXSHADER9	m_pVSAmbient;
	LPDIRECT3DVERTEXSHADER9	m_pVSDiffuse;
	LPDIRECT3DVERTEXDECLARATION9 m_pVSDecl;
	LPDIRECT3DPIXELSHADER9	m_pPS;

	//stencil shadow volume
	ShadowVolume m_shadowVolume;

	//bounding box size
	const static float SIZE_X;
	const static float SIZE_Y;
	const static float SIZE_Z;

	//physics simulation
	float		m_mass;
    D3DXMATRIX	m_matInverseTensor;	//inverse of the body-space inertia tensor

	D3DXVECTOR4	m_vPosition;
	D3DXVECTOR4	m_vLinearVelocity;
	D3DXMATRIX	m_matRotation;
	D3DXVECTOR4	m_vAngularMomentum;

	//auxiliary quantities
	D3DXVECTOR4 m_vAngularVelocity;
	D3DXMATRIX	m_matWorldTensor;	//world-space inertia tensor

	bool m_isOnGround;	//is the vehicle currently on the ground

	//collision grid on the base of the object
	const static int POINTS_PER_EDGE = 3;
	D3DXVECTOR4 m_collisionPoints[ POINTS_PER_EDGE ][ POINTS_PER_EDGE ];
	D3DXVECTOR3 m_collisionVectors[ POINTS_PER_EDGE ][ POINTS_PER_EDGE ];

};

#endif //INCLUSIONGUARD_VEHICLE_H
