//------------------------------------------------------------------------------
// File: Camera.h
// Desc: Object to represent a camera
//
// Created: 30 December 2002 17:20:55
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_CAMERA_H
#define INCLUSIONGUARD_CAMERA_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: class Camera
// Desc: The camera object
//------------------------------------------------------------------------------
class Camera
{
public:
	Camera();

	void SetCamera( const D3DXVECTOR3& vPos, const D3DXVECTOR3& vLookAt,
					const D3DXVECTOR3& vUp );
	inline const D3DXVECTOR3& GetPosition() const { return m_vPosition; }
	inline const D3DXVECTOR3& GetLookAtPt() const { return m_vLookAt; }
	inline const D3DXVECTOR3& GetUp() const { return m_vUp; }

	void SetProjection( const D3DXMATRIX& matProjection );
	inline const D3DXMATRIX& GetProjection() const { return m_matProjection; }

	inline const D3DXMATRIX& GetViewProj() const { return m_matViewProj; }

private:
	D3DXMATRIX m_matProjection;
	D3DXMATRIX m_matView;
	D3DXMATRIX m_matViewProj;

	D3DXVECTOR3 m_vPosition;
	D3DXVECTOR3 m_vLookAt;
	D3DXVECTOR3 m_vUp;

};


#endif //INCLUSIONGUARD_CAMERA_H
