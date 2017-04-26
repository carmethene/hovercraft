//------------------------------------------------------------------------------
// File: App.cpp
// Desc: The core application object
//
// Created: 30 December 2002 12:37:26
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <new>
#include <sstream>

#include "App.h"
#include "Backdrop.h"
#include "Camera.h"
#include "ChaseCam.h"
#include "Light.h"
#include "ParticleSystem.h"
#include "Scene.h"
#include "Sky.h"
#include "Terrain.h"
#include "Vehicle.h"

//used for memory-leak checking in debug builds
#if defined(_DEBUG) || defined(DEBUG)
#include "crtdbg.h"
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif


//------------------------------------------------------------------------------
// Macros:
//------------------------------------------------------------------------------
inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }


//------------------------------------------------------------------------------
// Constants:
//------------------------------------------------------------------------------
const DWORD HORIZON_COLOUR		= 0xff326496;
const DWORD HORIZON_COLOUR_HALF	= 0xff163248;
const float FAR_PLANE			= 350.0f;
const float FOG_START			= FAR_PLANE / 1.5f;
const float FOG_START_BG		= FAR_PLANE - 200.0f;
const float FOG_END				= FAR_PLANE;
const float CAMERA_NEAR			= 10.0f;
const float CAMERA_FAR			= 30.0f;
const float CAMERA_HEIGHT		= 8.0f;


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: App()
// Desc: Constructor for the application class
//------------------------------------------------------------------------------
App::App()
{
	//app framework settings
	m_strWindowTitle	= _T( "Hovercraft physics simulation - (c)2003 Neil Wakefield" );
	m_dwCreationHeight	= 600;
	m_dwCreationWidth	= 800;

	m_bStartFullscreen			= false;
	m_bShowCursorWhenFullscreen	= false;
	m_dx9Shaders				= false;
	m_twoSidedStencil			= false;

	m_d3dEnumeration.AppUsesDepthBuffer	= true;
	m_d3dEnumeration.AppMinStencilBits	= 4;

	//initialise member pointers
	m_pFont			= NULL;
	m_pParticles	= NULL;
	m_pTerrain		= NULL;
	m_pSky			= NULL;
	m_pBackdrop		= NULL;
	m_pVehicle		= NULL;
	m_pScene		= NULL;
	m_pCamera		= NULL;
	m_pChaseCam		= NULL;
	m_pDI			= NULL;
	m_pDIDKeyboard	= NULL;
	memset( m_diksOld, 0, sizeof( m_diksOld ) );
	m_pSoundManager	= NULL;
	m_pSNDEngine	= NULL;

	//set states
	m_pausePhysics		= false;
	m_showHelp			= false;
	m_drawWireframe		= false;
	m_showShadowVolumes	= false;
	m_cameraNear		= false;

	m_engineFrequency	= 22050;
}

//------------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Sets up application-specific data on first run
//------------------------------------------------------------------------------
HRESULT App::OneTimeSceneInit()
{
	//create a font
	try{ m_pFont = new CD3DFont( _T( "Arial" ), 12, D3DFONT_BOLD ); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}

	//create the scene geometry
	try{ m_pBackdrop = new Backdrop(); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}

	try{ m_pParticles = new ParticleSystem( 10000, 2.0f ); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}

	try{ m_pSky = new Sky(); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}

	try{ m_pTerrain = new Terrain(); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}

	try{ m_pVehicle = new Vehicle(); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}

	//set the initial vehicle parameters
	const float centerPoint = m_pTerrain->GetTerrainSize() / 2.0f;
	const float centerHeight = m_pTerrain->GetHeightMapPoint( centerPoint, centerPoint );
	m_pVehicle->SetPosition( D3DXVECTOR3( centerPoint, centerHeight + 2.0f, centerPoint ) );

	//set up the camera
	try{ m_pChaseCam = new ChaseCam( CAMERA_FAR, CAMERA_HEIGHT, 100.0f ); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}
	D3DXVECTOR3 vPosition = m_pVehicle->GetPosition();
	m_pChaseCam->SetChasePosition( vPosition );
	vPosition[ 0 ] -= 30.0f;
	vPosition[ 1 ] += 10.0f;
	vPosition[ 2 ] -= 30.0f;
	m_pChaseCam->SetCameraPosition( vPosition );

	try{ m_pCamera = new Camera(); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}
	m_pCamera->SetCamera( m_pChaseCam->GetCameraPosition(),
						  m_pChaseCam->GetChasePosition(),
						  D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) );

	D3DXMATRIX matProj;
	float fAspect = m_d3dsdBackBuffer.Width / float(m_d3dsdBackBuffer.Height);
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 1.0f, FAR_PLANE );
	m_pCamera->SetProjection( matProj );

	//create lights
	D3DXVECTOR3 vLightDirection( 5.0f, -5.0f, 5.0f );
	D3DXVec3Normalize( &vLightDirection, &vLightDirection );
	const Light lightSun( vLightDirection, D3DXVECTOR3( 0.6f, 0.6f, 0.6f ) );

	//set up the scene
	try{ m_pScene = new Scene(); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}
	m_pScene->SetCamera( *m_pCamera );
	m_pScene->SetAmbientLight( D3DXVECTOR3( 0.4f, 0.4f, 0.4f ) );
	m_pScene->AddLight( lightSun );
	
	//set up directinput
	if( FAILED( DirectInput8Create( GetModuleHandle( 0 ), DIRECTINPUT_VERSION,
									IID_IDirectInput8, (void**)&m_pDI, NULL ) ) )
		return E_FAIL;
	if( FAILED( m_pDI->CreateDevice( GUID_SysKeyboard, &m_pDIDKeyboard, NULL ) ) )
		return E_FAIL;
	if( FAILED( m_pDIDKeyboard->SetDataFormat( &c_dfDIKeyboard ) ) )
		return E_FAIL;
	if( FAILED( m_pDIDKeyboard->SetCooperativeLevel( m_hWnd, DISCL_EXCLUSIVE |
													 DISCL_FOREGROUND ) ) )
		return E_FAIL;

	//setup directsound
	try{ m_pSoundManager = new CSoundManager(); }
	catch( std::bad_alloc& error )
	{
		MessageBox( NULL, error.what(), "Error", MB_ICONEXCLAMATION | MB_OK );
		return E_OUTOFMEMORY;
	}
	if( FAILED( m_pSoundManager->Initialize( m_hWnd, DSSCL_PRIORITY ) ) )
		return E_FAIL;

	//load the engine sound
	HRSRC hRes;
	HGLOBAL hGlobal;
	LPVOID pData = NULL;
	ULONG dataSize = 0;
	HMODULE hModule = GetModuleHandle( NULL );
	
	WAVEFORMATEX wfx;
	wfx.cbSize = sizeof( WAVEFORMATEX );
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 22050;
	wfx.nAvgBytesPerSec = 22050;
	wfx.nBlockAlign = 1;
	wfx.wBitsPerSample = 8;

	//load the resource
	hRes = FindResource( hModule, MAKEINTRESOURCE( IDD_WAV_ENGINE ), "WAVE" );
	if( hRes == NULL )
		return E_FAIL;
	
	hGlobal = LoadResource( hModule, hRes );
	if( hGlobal == NULL )
		return E_FAIL;

	pData = LockResource( hGlobal );
	if( pData == NULL )
		return E_FAIL;

	dataSize = SizeofResource( hModule, hRes );

	//create the sound object
	const int headerSize = 44;
	const int footerSize = 234;

	if( FAILED( m_pSoundManager->CreateFromMemory( &m_pSNDEngine, ( (BYTE*)pData ) + headerSize,
												   dataSize - ( headerSize + footerSize ),
												   &wfx, DSBCAPS_CTRLVOLUME |
												   DSBCAPS_CTRLFREQUENCY ) ) )
		return E_FAIL;

	//unload the resource
	UnlockResource( hGlobal );
	FreeResource( hGlobal );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT App::InitDeviceObjects()
{
	//check card capabilities
	//vertex shader support
	m_dx9Shaders = ( m_d3dCaps.VertexShaderVersion >= D3DVS_VERSION( 2, 0 ) );
	if( ! m_dx9Shaders )
	{
		std::stringstream ss;
		ss << "Your video card does not support v2.0 shaders\n"
		   << "Graphics will not be correctly displayed\n"
		   << "You have been warned :)";
		MessageBox( NULL, ss.str().c_str(), "Warning", MB_OK | MB_ICONEXCLAMATION );
	}

	//two-sided stencil buffer
	m_twoSidedStencil = ( m_d3dCaps.StencilCaps & D3DSTENCILCAPS_TWOSIDED ) ? true : false;

	//hardware vertex processing
	if( m_d3dCaps.DeviceType != D3DDEVTYPE_HAL )
		m_dx9Shaders = false;

	//init the scene geometry
	if( FAILED( m_pBackdrop->InitDeviceObjects( m_pd3dDevice, m_dx9Shaders,
												m_twoSidedStencil ) ) )
		return E_FAIL;
	if( FAILED( m_pParticles->InitDeviceObjects( m_pd3dDevice, m_dx9Shaders,
												 m_twoSidedStencil ) ) )
		return E_FAIL;
	if( FAILED( m_pSky->InitDeviceObjects( m_pd3dDevice, m_dx9Shaders,
										   m_twoSidedStencil ) ) )
		return E_FAIL;
	if( FAILED( m_pTerrain->InitDeviceObjects( m_pd3dDevice, m_dx9Shaders,
											   m_twoSidedStencil ) ) )
		return E_FAIL;
	if( FAILED( m_pVehicle->InitDeviceObjects( m_pd3dDevice, m_dx9Shaders,
											   m_twoSidedStencil ) ) )
		return E_FAIL;

	//initialise the font
	if( FAILED( m_pFont->InitDeviceObjects( m_pd3dDevice ) ) )
		return E_FAIL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: RestoreDeviceObjects
// Desc: Sets up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT App::RestoreDeviceObjects()
{
	//restore the scene geometry
	if( FAILED( m_pBackdrop->RestoreDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pParticles->RestoreDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pSky->RestoreDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pTerrain->RestoreDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pVehicle->RestoreDeviceObjects() ) )
		return E_FAIL;

	//restore the font
	if( FAILED( m_pFont->RestoreDeviceObjects() ) )
		return E_FAIL;

	//set render states
	m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );	//z-buffering on
	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );		//no d3d lighting
	m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE, TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_LINEAR );
	m_pd3dDevice->SetRenderState( D3DRS_FOGVERTEXMODE, D3DFOG_NONE );
	m_pd3dDevice->SetRenderState( D3DRS_FOGSTART, FtoDW( FOG_START ) );
	m_pd3dDevice->SetRenderState( D3DRS_FOGEND, FtoDW( FOG_END ) );
	m_pd3dDevice->SetRenderState( D3DRS_FOGCOLOR, HORIZON_COLOUR );

	//set trilinear texture filtering
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	//store the new screen size in the camera
	D3DXMATRIX matProj;
	float fAspect = m_d3dsdBackBuffer.Width / float(m_d3dsdBackBuffer.Height);
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 1.0f, FAR_PLANE );
	m_pCamera->SetProjection( matProj );
	m_pScene->SetCamera( *m_pCamera );
    
	//restore DirectInput
	m_pDIDKeyboard->Acquire();

	//start the engine sound
	m_pSNDEngine->Play( 0, DSBPLAY_LOOPING );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the current frame
//------------------------------------------------------------------------------
HRESULT App::Render()
{
	//blank the screen
	if( FAILED( m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER |
									 D3DCLEAR_STENCIL, HORIZON_COLOUR, 1.0f, 0 ) ) )
		return E_FAIL;

	//render the scene...
	if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
	{
		//render background (sky and sea)
		m_pd3dDevice->SetRenderState( D3DRS_FOGCOLOR, HORIZON_COLOUR );
		m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_FOGSTART, FtoDW( FOG_START_BG ) );
		m_pBackdrop->Render( *m_pScene );
		m_pSky->Render( *m_pScene );
		m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
		m_pd3dDevice->SetRenderState( D3DRS_FOGSTART, FtoDW( FOG_START ) );
	
		//half fog colour, as we will draw this twice via additive blending
		m_pd3dDevice->SetRenderState( D3DRS_FOGCOLOR, HORIZON_COLOUR_HALF );

		//do ambient lighting pass, with shadow volumes where appropriate
		m_pTerrain->Render( *m_pScene, false );
		m_pVehicle->Render( *m_pScene, false, true );

		//do diffuse lighting pass (additive blending)
		m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, TRUE );
		m_pd3dDevice->SetRenderState( D3DRS_STENCILREF,  0x1 );
		m_pd3dDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_GREATER );
		m_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
		m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_EQUAL );

		m_pTerrain->Render( *m_pScene, true );
		m_pVehicle->Render( *m_pScene, true, false );

		m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
		m_pd3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

		//render the dust trail
		m_pParticles->Render( *m_pScene );

		//render the statistics
		m_pFont->DrawText( 5.0f, 5.0f, 0xccffff00, m_strDeviceStats );
		m_pFont->DrawText( 5.0f, 25.0f, 0xccffff00, m_strFrameStats );

		std::stringstream ss;
		ss << "Visible terrain cells: " << m_pTerrain->GetVisibleCells();
		m_pFont->DrawText( 5.0f, 45.0f, 0xccffff00, ss.str().c_str() );

		//render the help
		if( ! m_showHelp )
		{
			m_pFont->DrawText( 5.0f, 65.0f, 0xccffff00, "Press F1 to show help" );
		}
		else
		{
			m_pFont->DrawText( 5.0f, 65.0f, 0xccffff00, "Press F1 to hide help" );
			m_pFont->DrawText( 5.0f, 105.0f, 0xccffff00, "Up / Down" );
			m_pFont->DrawText( 105.0f, 105.0f, 0xccffff00, "Forwards / Backwards" );
			m_pFont->DrawText( 5.0f, 125.0f, 0xccffff00, "Left / Right" );
			m_pFont->DrawText( 105.0f, 125.0f, 0xccffff00, "Turn" );
			m_pFont->DrawText( 5.0f, 145.0f, 0xccffff00, "Space" );
			m_pFont->DrawText( 105.0f, 145.0f, 0xccffff00, "Center camera (hold)" );
			m_pFont->DrawText( 5.0f, 165.0f, 0xccffff00, "P" );
			m_pFont->DrawText( 105.0f, 165.0f, 0xccffff00, "Pause physics simulation" );
			m_pFont->DrawText( 5.0f, 185.0f, 0xccffff00, "C" );
			m_pFont->DrawText( 105.0f, 185.0f, 0xccffff00, "Toggle camera distance" );
			m_pFont->DrawText( 5.0f, 205.0f, 0xccffff00, "F" );
			m_pFont->DrawText( 105.0f, 205.0f, 0xccffff00, "Toggle fullscreen/window" );
			m_pFont->DrawText( 5.0f, 225.0f, 0xccffff00, "W" );
			m_pFont->DrawText( 105.0f, 225.0f, 0xccffff00, "Toggle wireframe mode" );
			m_pFont->DrawText( 5.0f, 245.0f, 0xccffff00, "S" );
			m_pFont->DrawText( 105.0f, 245.0f, 0xccffff00, "Show shadow volumes" );
		}

		m_pd3dDevice->EndScene();
	}

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Performs between-frame animation
//------------------------------------------------------------------------------
HRESULT App::FrameMove()
{
	//HACK: cap time interval to stop calculation errors at startup
	//warning - if the framerate drops below 10fps the camera will no longer follow
	if( m_fElapsedTime > 0.1f )
		m_fElapsedTime = 0.1f;
	//END HACK

	//set the time for the procedural sky
	m_pSky->SetTime( m_fTime );

	//get the keyboard state
	bool forwardThrust	= false;
	bool reverseThrust	= false;
	bool leftThrust		= false;
	bool rightThrust	= false;
	bool centerCam		= false;

	unsigned char diks[ 256 ];
	memset( &diks, 0, sizeof( diks ) );

	if( FAILED( m_pDIDKeyboard->GetDeviceState( sizeof( diks ), &diks ) ) )
	{
		m_pDIDKeyboard->Unacquire();
		m_pDIDKeyboard->Acquire();
	}
	else
	{
		//player vehicle controls
		forwardThrust	= ( diks[ DIK_UP ] & 0x80 ) ? true : false;
		reverseThrust	= ( diks[ DIK_DOWN ] & 0x80 ) ? true : false;
		leftThrust		= ( diks[ DIK_LEFT ] & 0x80 ) ? true : false;
		rightThrust		= ( diks[ DIK_RIGHT ] & 0x80 ) ? true : false;
		centerCam		= ( diks[ DIK_SPACE ] & 0x80 ) ? true : false;

		//misc states...
		//pause mode
		if( ( diks[ DIK_P ] & 0x80 ) && !( m_diksOld[ DIK_P ] & 0x80 ) )
		{
			m_pausePhysics = ! m_pausePhysics;
		}

		//help display
		if( ( diks[ DIK_F1 ] & 0x80 ) && !( m_diksOld[ DIK_F1 ] & 0x80 ) )
		{
			m_showHelp = ! m_showHelp;
		}

		//camera distance
		if( ( diks[ DIK_C ] & 0x80 ) && !( m_diksOld[ DIK_C ] & 0x80 ) )
		{
			m_cameraNear = ! m_cameraNear;
			if( m_cameraNear )
				m_pChaseCam->SetParameters( CAMERA_NEAR, CAMERA_HEIGHT );
			else
				m_pChaseCam->SetParameters( CAMERA_FAR, CAMERA_HEIGHT );
		}

		//wireframe mode
		if( ( diks[ DIK_W ] & 0x80 ) && !( m_diksOld[ DIK_W ] & 0x80 ) )
		{
			m_drawWireframe = ! m_drawWireframe;

			if( m_drawWireframe )
				m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
			else
				m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
		}

		//shadow volumes
		if( ( diks[ DIK_S ] & 0x80 ) && !( m_diksOld[ DIK_S ] & 0x80 ) )
		{
			m_showShadowVolumes = ! m_showShadowVolumes;
		}

		//full-screen mode
		if( ( diks[ DIK_F ] & 0x80 ) && !( m_diksOld[ DIK_F ] & 0x80 ) )
		{
			SendMessage( m_hWnd, WM_COMMAND, IDM_TOGGLEFULLSCREEN, 0 );
		}

		memcpy( m_diksOld, diks, sizeof( diks ) );
	}

	if( ! m_pausePhysics )
	{
		//run the physics simulation on the vehicle
		m_pVehicle->DoPhysics( m_fElapsedTime, m_pTerrain, forwardThrust, reverseThrust,
							leftThrust, rightThrust );

		const D3DXVECTOR3 vVehiclePosition	= m_pVehicle->GetPosition();
		const D3DXVECTOR3 vVehicleDirection	= m_pVehicle->GetDirection();
		const D3DXVECTOR3 vVehicleVelocity	= m_pVehicle->GetVelocity();

		//update the vehicle's dust trail...
		//particle generation position
		D3DXVECTOR3 particlePosition = vVehiclePosition;
		particlePosition.y -= 1.8f;
		m_pParticles->SetPosition( particlePosition );

		//particle velocity
		if( m_pVehicle->IsOnGround() )
		{
			m_pParticles->SetVelocity( - vVehicleVelocity );
			m_pParticles->UpdateParticles( m_fElapsedTime );
		}
		else
		{
			m_pParticles->SetVelocity( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
		}

		//update the chasecam
		m_pChaseCam->SetChasePosition( vVehiclePosition );
		m_pChaseCam->SetChaseDirection( vVehicleDirection );
		m_pChaseCam->SetChaseVelocity( vVehicleVelocity );
		m_pChaseCam->SetCameraVelocity( vVehicleVelocity );
		m_pChaseCam->UpdatePosition( m_fElapsedTime, centerCam );

		//adjust height to make sure the camera follows the terrain
		D3DXVECTOR3 vPos = m_pChaseCam->GetCameraPosition();
		float height = m_pTerrain->GetHeightMapPoint( vPos[ 0 ], vPos[ 2 ] );
		height = vPos[ 1 ] - ( height + 3.0f );
		if( height < 0.0f )
			vPos[ 1 ] -= height;
		
		//update the camera position
		m_pCamera->SetCamera( vPos, m_pChaseCam->GetChasePosition(),
 							  D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) );
		m_pScene->SetCamera( *m_pCamera );

		//camera has moved so cull terrain
		m_pTerrain->CullQuadtree( *m_pScene );

		//vary the engine volume to match the vehicle distance
		D3DXVECTOR3 viewVector = vVehiclePosition - vPos;
		float vehicleDistance = D3DXVec3Length( &viewVector );
		const int volume = 0 - int( vehicleDistance * 30.0f );
		m_pSNDEngine->GetBuffer( 0 )->SetVolume( volume );

		//vary the engine frequency with the vehicle power and height...
		//there are three conditions:
		//1) no engine thrust, low frequency
		//2) engine thrust and on ground, mid frequency
		//3) engine thrust and in the air, high frequency
		int targetFrequency;
		if( ( ! forwardThrust ) && ( ! reverseThrust ) )
			targetFrequency = 22050;
		else if( m_pVehicle->IsOnGround() )
			targetFrequency = 23500;
		else
			targetFrequency = 24500;

		//fade the current frequency towards the target frequency...
		if( m_engineFrequency != targetFrequency )
		{
			//what is the maximum we can vary the frequency by for this frame?
			const int frequencyStep = int( 3500.0f * m_fElapsedTime );

            //how far away are we from the target frequency?
			const int frequencyDifference = m_engineFrequency - targetFrequency;

			//if we are close to the target frequency, simply jump to it
			if( abs( frequencyDifference ) <= frequencyStep )
				m_engineFrequency = targetFrequency;
			else
			{
				//otherwise, move the engine frequency towards the target frequency
				if( frequencyDifference > 0 )
					m_engineFrequency -= frequencyStep;
				else
					m_engineFrequency += frequencyStep;
			}
		}

		m_pSNDEngine->GetBuffer( 0 )->SetFrequency( m_engineFrequency );
	}

	//update vehicle shadow volume
	m_pVehicle->UpdateShadowVolume( *m_pScene, m_showShadowVolumes );
		
	//store the view projection matrix - needed for correct fog
	D3DXMATRIX matProj = m_pCamera->GetProjection();
	m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    return S_OK;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT App::InvalidateDeviceObjects()
{
	//invalidate the scene geometry
	if( FAILED( m_pBackdrop->InvalidateDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pParticles->InvalidateDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pSky->InvalidateDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pTerrain->InvalidateDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pVehicle->InvalidateDeviceObjects() ) )
		return E_FAIL;

	//invalidate the font
	if( FAILED(	m_pFont->InvalidateDeviceObjects() ) )
		return E_FAIL;

	//invalidate DirectInput
	m_pDIDKeyboard->Unacquire();

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT App::DeleteDeviceObjects()
{
	//delete the scene geometry
	if( FAILED( m_pBackdrop->DeleteDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pParticles->DeleteDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pSky->DeleteDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pTerrain->DeleteDeviceObjects() ) )
		return E_FAIL;
	if( FAILED( m_pVehicle->DeleteDeviceObjects() ) )
		return E_FAIL;

	//delete the font geometry
	if( FAILED(	m_pFont->DeleteDeviceObjects() ) )
		return E_FAIL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Tidies up application-specific data on shutdown
//------------------------------------------------------------------------------
HRESULT App::FinalCleanup()
{
	//tidy up the scene geometry
	SAFE_DELETE( m_pBackdrop );
	SAFE_DELETE( m_pParticles );
	SAFE_DELETE( m_pSky );
	SAFE_DELETE( m_pTerrain );
	SAFE_DELETE( m_pVehicle );
	SAFE_DELETE( m_pScene );
	SAFE_DELETE( m_pCamera );
	SAFE_DELETE( m_pChaseCam );

	//tidy up the font
	SAFE_DELETE( m_pFont );

	//tidy up directinput
	SAFE_RELEASE( m_pDIDKeyboard );
	SAFE_RELEASE( m_pDI );

	//tidy up directsound
	m_pSNDEngine->Stop();
	SAFE_DELETE( m_pSNDEngine );
	SAFE_DELETE( m_pSoundManager );

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Checks the device for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT App::ConfirmDevice( D3DCAPS9* pCaps, DWORD behavior, D3DFORMAT, D3DFORMAT )
{
	UNREFERENCED_PARAMETER( behavior );
	UNREFERENCED_PARAMETER( pCaps );

	//NOTE: Disabled checking - it should run on any version, with hardware accel on
	//vs1.1 and above, but pixel shaders will not be used unless v2.0 is supported,
	//which means it'll look awful

	/*
	//vertex shader support
	if( ( behavior & D3DCREATE_HARDWARE_VERTEXPROCESSING ) ||
		( behavior & D3DCREATE_MIXED_VERTEXPROCESSING ) )
	{
		if( pCaps->VertexShaderVersion < D3DVS_VERSION( 2, 0 ) )
		{
			return E_FAIL;
		}
	}

	//two-sided stencil buffer
	if( ! ( pCaps->StencilCaps & D3DSTENCILCAPS_TWOSIDED ) )
	{
		return E_FAIL;
	}
	*/

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the application
//------------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, INT )
{
	//enable memory-leak checking in debug builds
	#if defined(_DEBUG) || defined(DEBUG)
	int flag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	flag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag( flag );
	#endif

	App theApp;
	theApp.Create( hInstance );
	return theApp.Run();
}
