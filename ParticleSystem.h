//------------------------------------------------------------------------------
// File: ParticleSystem.h
// Desc: Representation of a particle system whose particles are under the
//		 influence of gravity, and whose particles have a constant lifetime
//
// Created: 25 January 2003 12:11:05
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_PARTICLESYSTEM_H
#define INCLUSIONGUARD_PARTICLESYSTEM_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class Scene;

//------------------------------------------------------------------------------
// Name: class ParticleSystem
// Desc: The particle system
//------------------------------------------------------------------------------
class ParticleSystem
{
public:
	ParticleSystem( const int numParticles, const float particleLifetime );
	~ParticleSystem();

	HRESULT InitDeviceObjects( const LPDIRECT3DDEVICE9 pd3dDevice, const bool dx9Shaders,
							   const bool twoSidedStencil );
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT DeleteDeviceObjects();

	HRESULT Render( const Scene& scene ) const;

	HRESULT UpdateParticles( const float timeStep );

	inline void SetPosition( const D3DXVECTOR3 position ) { m_initialPosition = position; }
	inline void SetVelocity( const D3DXVECTOR3 velocity )
	{
		D3DXVECTOR3 particleVelocity = velocity;
		D3DXVec3Normalize( &particleVelocity, &particleVelocity );
		m_initialVelocity = particleVelocity;
		m_initialSpeed = ( velocity.x * velocity.x ) +
						 ( velocity.y * velocity.y ) +
						 ( velocity.z * velocity.z );
		m_initialSpeed = float( sqrt( m_initialSpeed ) );
	}

private:
	void InitParticles();

	inline float ParticleSystem::frand( const int seed )
	{
		const int x = ( seed << 13 ) ^ seed;
		return ( 2.0f - ( ( x * ( x * x * 15731 + 789221 ) + 1376312589 ) & 0x7fffffff )
				/ 1073741824.0f ) * 0.5f;
	}

	inline double gaussianRand( const float mean, const float sd )
	{
		double x2pi = frand( rand() ) * 2.0f * D3DX_PI;
		double g2rad = sqrt( -2.0f * log( 1.0f - frand( rand() ) ) );
		return mean + cos( x2pi ) * g2rad * sd;
	}

	//initial particle parameters
	unsigned int	m_numParticles;
	D3DXVECTOR3		m_initialPosition;
	D3DXVECTOR3		m_initialVelocity;
	float			m_initialSpeed;
	D3DXVECTOR3		m_gravity;
	float			m_particleLifetime;

	//per-particle parameters
    D3DXVECTOR3*	m_particlePositions;
	D3DXVECTOR3*	m_particleVelocities;
	float*			m_particleAges;

	//direct3d objects
	LPDIRECT3DDEVICE9				m_pd3dDevice;
	LPDIRECT3DVERTEXDECLARATION9	m_pVSDecl;
	LPDIRECT3DVERTEXSHADER9			m_pVS;

};


#endif //INCLUSIONGUARD_PARTICLESYSTEM_H
