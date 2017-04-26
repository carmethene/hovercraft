//------------------------------------------------------------------------------
// File: ChaseCam.h
// Desc: Chase camera with spring system
//
// Created: 09 January 2003 00:14:48
//
// Stolen from Simon Brown's buggy demo, with permission :)
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_CHASECAM_H
#define INCLUSIONGUARD_CHASECAM_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>
#include <sstream>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: class ChaseCam
// Desc: A simple chasecam class
//------------------------------------------------------------------------------
class ChaseCam
{
public:
	ChaseCam( const float followDistance, const float followHeight, const float omegaSq )
	{
		m_followDistance	= followDistance;
		m_followHeight		= followHeight;
		m_omegaSq			= omegaSq;

		//critical damping
		m_beta = 2.0f * float( sqrt( omegaSq ) );
	}

    void UpdatePosition( const float timeInterval, const bool useDirection )
	{
		D3DXVECTOR3 a;

		//compute the position-dependent acceleration
		if( useDirection )
		{
			a[ 0 ] = -m_omegaSq * ( m_vCameraPosition[ 0 ] - ( m_vChasePosition[ 0 ] -
									m_followDistance * m_vChaseDirection[ 0 ] ) );
            a[ 1 ] = -m_omegaSq * ( m_vCameraPosition[ 1 ] - ( m_vChasePosition[ 1 ] +
									m_followHeight ) );
			a[ 2 ] = -m_omegaSq * ( m_vCameraPosition[ 2 ] - ( m_vChasePosition[ 2 ] -
									m_followDistance * m_vChaseDirection[ 2 ] ) );			
		}
		else
		{
			float r = float( sqrt( m_vChaseVelocity[ 0 ] * m_vChaseVelocity[ 0 ] +
								   m_vChaseVelocity[ 2 ] * m_vChaseVelocity[ 2 ] ) );

			if( r > 2.5f ) // > 2.5m/s
			{
				a[ 0 ] = -m_omegaSq * ( m_vCameraPosition[ 0 ] - ( m_vChasePosition[ 0 ] -
									  m_followDistance * m_vChaseVelocity[ 0 ] / r ) );
				a[ 1 ] = -m_omegaSq * ( m_vCameraPosition[ 1 ] - ( m_vChasePosition[ 1 ] +
									  m_followHeight ) );
				a[ 2 ] = -m_omegaSq * ( m_vCameraPosition[ 2 ] - ( m_vChasePosition[ 2 ] -
									  m_followDistance * m_vChaseVelocity[ 2 ] / r ) );				
			}
			else
			{
				D3DXVECTOR3 offset = m_vCameraPosition - m_vChasePosition;
				r = float( sqrt( offset[ 0 ] * offset[ 0 ] + offset[ 2 ] * offset[ 2 ] ) );
				if( r > 0.001f ) // > 1mm
				{
					float accel = -m_omegaSq * ( r - m_followDistance ) / r;
					a[ 0 ] = offset[ 0 ] * accel;
					a[ 2 ] = offset[ 2 ] * accel;
				}
				a[ 1 ] = -m_omegaSq * ( offset[ 1 ] - m_followHeight );
			}
		}

		// compute the velocity-dependent acceleration
		a -= m_beta * ( m_vCameraVelocity - m_vChaseVelocity );

		// update by one timeInterval
		m_vCameraVelocity += a * timeInterval;
		m_vCameraPosition += m_vCameraVelocity * timeInterval;
	}

	void SetChasePosition( const D3DXVECTOR3& vChasePosition )
	{
		m_vChasePosition = vChasePosition;
	}

	void SetChaseVelocity( const D3DXVECTOR3& vChaseVelocity )
	{
		m_vChaseVelocity = vChaseVelocity;
	}

	void SetChaseDirection( const D3DXVECTOR3& vChaseDirection )
	{
		m_vChaseDirection = vChaseDirection;
	}
	
	void SetCameraPosition( const D3DXVECTOR3& vCameraPosition )
	{
		m_vCameraPosition = vCameraPosition;
	}

	void SetCameraVelocity( const D3DXVECTOR3& vCameraVelocity )
	{
		m_vCameraVelocity = vCameraVelocity;
	}

	const D3DXVECTOR3 GetChasePosition() const { return m_vChasePosition; }
	const D3DXVECTOR3 GetCameraPosition() const { return m_vCameraPosition; }

	void SetParameters( const float followDistance, const float followHeight )
	{
		m_followDistance	= followDistance;
		m_followHeight		= followHeight;
	}
	
private:
	D3DXVECTOR3 m_vChasePosition;
	D3DXVECTOR3 m_vChaseVelocity;
	D3DXVECTOR3 m_vChaseDirection;
	D3DXVECTOR3 m_vCameraPosition;
	D3DXVECTOR3 m_vCameraVelocity;

	float m_followDistance;
	float m_followHeight;

private:
	float m_omegaSq;
	float m_beta;

};


#endif //INCLUSIONGUARD_CHASECAM_H
