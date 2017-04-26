//------------------------------------------------------------------------------
// File: Frustum.h
// Desc: Frustum representation, and generation functions
//
// Created: 03 January 2003 21:42:38
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_FRUSTUM_H
#define INCLUSIONGUARD_FRUSTUM_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: struct Frustum
// Desc: Representation of a view frustum
//------------------------------------------------------------------------------
struct Frustum
{
	D3DXPLANE planes[ 6 ];
};

Frustum ExtractFrustum( const D3DMATRIX matrix, const bool normalise );


#endif //INCLUSIONGUARD_FRUSTUM_H
