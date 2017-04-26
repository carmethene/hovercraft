//------------------------------------------------------------------------------
// File: Frustum.cpp
// Desc: Frustum representation, and generation functions
//
// Created: 03 January 2003 22:04:58
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "Frustum.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: ExtractFrustum()
// Desc: Generates a frustum from a view/projection transform matrix
//------------------------------------------------------------------------------
Frustum ExtractFrustum( const D3DMATRIX matrix, const bool normalise )
{
	Frustum frustum;

	//left clipping plane
	frustum.planes[0].a = - ( matrix._14 + matrix._11 );
	frustum.planes[0].b = - ( matrix._24 + matrix._21 );
	frustum.planes[0].c = - ( matrix._34 + matrix._31 );
	frustum.planes[0].d = - ( matrix._44 + matrix._41 );

	//right clipping plane
	frustum.planes[1].a = - ( matrix._14 - matrix._11 );
	frustum.planes[1].b = - ( matrix._24 - matrix._21 );
	frustum.planes[1].c = - ( matrix._34 - matrix._31 );
	frustum.planes[1].d = - ( matrix._44 - matrix._41 );

	//top clipping plane
	frustum.planes[2].a = - ( matrix._14 - matrix._12 );
	frustum.planes[2].b = - ( matrix._24 - matrix._22 );
	frustum.planes[2].c = - ( matrix._34 - matrix._32 );
	frustum.planes[2].d = - ( matrix._44 - matrix._42 );

	//bottom clipping plane
	frustum.planes[3].a = - ( matrix._14 + matrix._12 );
	frustum.planes[3].b = - ( matrix._24 + matrix._22 );
	frustum.planes[3].c = - ( matrix._34 + matrix._32 );
	frustum.planes[3].d = - ( matrix._44 + matrix._42 );

	//near clipping plane
	frustum.planes[4].a = - matrix._13;
	frustum.planes[4].b = - matrix._23;
	frustum.planes[4].c = - matrix._33;
	frustum.planes[4].d = - matrix._43;

	//far clipping plane
	frustum.planes[5].a = - ( matrix._14 - matrix._13 );
	frustum.planes[5].b = - ( matrix._24 - matrix._23 );
	frustum.planes[5].c = - ( matrix._34 - matrix._33 );
	frustum.planes[5].d = - ( matrix._44 - matrix._43 );

	//normalise
	if( normalise )
	{
		D3DXPlaneNormalize( &frustum.planes[0], &frustum.planes[0] );
		D3DXPlaneNormalize( &frustum.planes[1], &frustum.planes[1] );
		D3DXPlaneNormalize( &frustum.planes[2], &frustum.planes[2] );
		D3DXPlaneNormalize( &frustum.planes[3], &frustum.planes[3] );
		D3DXPlaneNormalize( &frustum.planes[4], &frustum.planes[4] );
		D3DXPlaneNormalize( &frustum.planes[5], &frustum.planes[5] );
	}

	return frustum;
}
