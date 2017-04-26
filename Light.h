//------------------------------------------------------------------------------
// File: Light.h
// Desc: Class to represent a light source
//
// Created: 31 December 2002 09:00:21
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_LIGHT_H
#define INCLUSIONGUARD_LIGHT_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: class Light
// Desc: The lightsource class (currently pointlights only)
//------------------------------------------------------------------------------
class Light
{
public:
	Light();
	Light( const D3DXVECTOR3& vPosition, const D3DXVECTOR3& vColour );

	inline void SetPosition( const D3DXVECTOR3& vPosition ) { m_vPosition = vPosition; }
	inline const D3DXVECTOR3& GetPosition() const { return m_vPosition; }

	inline void SetColour( const D3DXVECTOR3& vColour ) { m_vColour = vColour; }
	inline const D3DXVECTOR3& GetColour() const { return m_vColour; }

private:
	D3DXVECTOR3 m_vPosition;
	D3DXVECTOR3 m_vColour;

};


#endif //INCLUSIONGUARD_LIGHT_H
