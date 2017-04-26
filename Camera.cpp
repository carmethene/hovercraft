//------------------------------------------------------------------------------
// File: Camera.cpp
// Desc: Object to represent a camera
//
// Created: 30 December 2002 22:48:45
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "Camera.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: Camera()
// Desc: Construtor for the camera object
//------------------------------------------------------------------------------
Camera::Camera()
{
	D3DXMatrixIdentity( &m_matView );
	D3DXMatrixIdentity( &m_matProjection );
	D3DXMatrixIdentity( &m_matViewProj );
}

//------------------------------------------------------------------------------
// Name: SetCamera()
// Desc: Creates the view transformation matrix for the camera
//------------------------------------------------------------------------------
void Camera::SetCamera( const D3DXVECTOR3& vPos, const D3DXVECTOR3& vLookAt,
					    const D3DXVECTOR3& vUp )
{
	m_vPosition	= vPos;
	m_vLookAt	= vLookAt;
	m_vUp		= vUp;

	D3DXMatrixLookAtLH( &m_matView, &m_vPosition, &m_vLookAt, &m_vUp );
	D3DXMatrixMultiply( &m_matViewProj, &m_matView, &m_matProjection );	
}

//------------------------------------------------------------------------------
// Name: SetProjection()
// Desc: Stores a projection transformation matrix for the screen
//------------------------------------------------------------------------------
void Camera::SetProjection( const D3DXMATRIX& matProjection )
{
	m_matProjection = matProjection;
	D3DXMatrixMultiply( &m_matViewProj, &m_matView, &m_matProjection );
}
