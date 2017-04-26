//------------------------------------------------------------------------------
// File: Scene.h
// Desc: Structure to hold information on the current scene which is needed for
//		 rendering
//
// Created: 31 December 2002 08:59:39
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_SCENE_H
#define INCLUSIONGUARD_SCENE_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>

#include "Camera.h"
#include "Light.h"


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: class Scene
// Desc: The scene information class
//------------------------------------------------------------------------------
class Scene
{
public:
	const static int MAX_LIGHTS = 8;

	Scene();

	inline void SetAmbientLight( const D3DXVECTOR3& vAmbientColour )
	{
		m_vAmbientLight = vAmbientColour;
	}
	inline const D3DXVECTOR3& GetAmbientLight() const { return m_vAmbientLight; }

	bool AddLight( const Light& light );
	int GetNumLights() const { return m_numLights; }
	const Light& GetLight( const int lightNum ) const;

	inline void SetCamera( const Camera& camera ) { m_camera = camera; }
	inline const Camera& GetCamera() const { return m_camera; }

private:
	//ambient lighting colour
	D3DXVECTOR3 m_vAmbientLight;

	//lights in the scene
	int m_numLights;
	Light m_lights[ MAX_LIGHTS ];

	//camera
	Camera m_camera;
};

#endif //INCLUSIONGUARD_SCENE_H
