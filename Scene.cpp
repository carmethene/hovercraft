//------------------------------------------------------------------------------
// File: Scene.cpp
// Desc: Structure to hold information on the current scene which is needed for
//		 rendering
//
// Created: 31 December 2002 09:33:37
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "Scene.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: Scene()
// Desc: Constructor for the scene object
//------------------------------------------------------------------------------
Scene::Scene()
{
	m_vAmbientLight	= D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	m_numLights		= 0;
}

//------------------------------------------------------------------------------
// Name: AddLight()
// Desc: Adds a light to the scene
//------------------------------------------------------------------------------
bool Scene::AddLight( const Light& light )
{
	if( m_numLights >= MAX_LIGHTS )
		return false;	//can't hold more lights

	m_lights[ m_numLights ] = light;
	++m_numLights;

	return true;
}

//------------------------------------------------------------------------------
// Name: GetLight()
// Desc: Retrieves a scene light
//------------------------------------------------------------------------------
const Light& Scene::GetLight( const int lightNum ) const
{
	if( lightNum >= MAX_LIGHTS )
		return m_lights[ 0 ];

	return m_lights[ lightNum ];
}
