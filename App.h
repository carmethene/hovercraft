//------------------------------------------------------------------------------
// File: App.h
// Desc: The core application object
//
// Created: 30 December 2002 12:37:16
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_APP_H
#define INCLUSIONGUARD_APP_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <dsound.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>
#include <dinput.h>
#include "DXUtil.h"
#include "DSUtil.h"
#include "D3DUtil.h"
#include "D3DEnumeration.h"
#include "D3DSettings.h"
#include "D3DApp.h"
#include "D3DRes.h"
#include "D3DFont.h"
#include "Resource.h"


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class Backdrop;
class Camera;
class ChaseCam;
class Scene;
class Sky;
class Terrain;
class Vehicle;
class ParticleSystem;

//------------------------------------------------------------------------------
// Name: class App
// Desc: The main application class
//------------------------------------------------------------------------------
class App : public CD3DApplication
{
public:
	App();

	HRESULT OneTimeSceneInit();
	HRESULT FinalCleanup();
	HRESULT InitDeviceObjects();
	HRESULT DeleteDeviceObjects();
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT Render();
	HRESULT FrameMove();
	HRESULT ConfirmDevice( D3DCAPS9* pCaps, DWORD behavior,
						   D3DFORMAT adaptorFormat, D3DFORMAT backBufferFormat );

private:
	CD3DFont*			m_pFont;
	LPDIRECT3DTEXTURE9	m_pShadowTexture;

	bool m_dx9Shaders;
	bool m_twoSidedStencil;

	//directinput
	LPDIRECTINPUT8			m_pDI;
	LPDIRECTINPUTDEVICE8	m_pDIDKeyboard;
	unsigned char			m_diksOld[ 256 ];

	//directsound
	CSoundManager*	m_pSoundManager;
	CSound*			m_pSNDEngine;
	DWORD			m_engineFrequency;
	
	//app states
	bool m_pausePhysics;
	bool m_showHelp;
	bool m_drawWireframe;
	bool m_showShadowVolumes;
	bool m_cameraNear;

	//scene geometry
	Backdrop*		m_pBackdrop;
	ParticleSystem*	m_pParticles;
	Sky*			m_pSky;
	Terrain*		m_pTerrain;	
	Vehicle*		m_pVehicle;

	//selection of cameras for the scene
	Scene*		m_pScene;
	ChaseCam*	m_pChaseCam;
	Camera*		m_pCamera;

};


#endif //INCLUSIONGUARD_APP_H
