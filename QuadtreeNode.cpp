//------------------------------------------------------------------------------
// File: QuadtreeNode.cpp
// Desc: A quadtree node for terrain that can be recursively tested against
//		 the view frustum and rendered if visible
//
// Created: 03 January 2003 10:27:37
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "QuadtreeNode.h"
#include "Frustum.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: QuadtreeNode()
// Desc: Constructor for the quadtree node class
//------------------------------------------------------------------------------
QuadtreeNode::QuadtreeNode()
{
	m_pChildren[ 0 ] = NULL;
	m_pChildren[ 1 ] = NULL;
	m_pChildren[ 2 ] = NULL;
	m_pChildren[ 3 ] = NULL;
	m_aabbMin[ 0 ]	 = 0.0f;
	m_aabbMin[ 1 ]	 = 0.0f;
	m_aabbMin[ 2 ]	 = 0.0f;
	m_aabbMax[ 0 ]	 = 0.0f;
	m_aabbMax[ 1 ]	 = 0.0f;
	m_aabbMax[ 2 ]	 = 0.0f;
	m_baseVertex	 = 0;
	m_isLeafNode	 = true;
}

QuadtreeNode::QuadtreeNode( QuadtreeNode* pChild1, QuadtreeNode* pChild2,
							QuadtreeNode* pChild3, QuadtreeNode* pChild4,
							const float minX, const float maxX,
							const float minY, const float maxY,
							const float minZ, const float maxZ )
{
	m_pChildren[ 0 ] = pChild1;
	m_pChildren[ 1 ] = pChild2;
	m_pChildren[ 2 ] = pChild3;
	m_pChildren[ 3 ] = pChild4;
	m_aabbMin[ 0 ]	 = minX;
	m_aabbMin[ 1 ]	 = minY;
	m_aabbMin[ 2 ]	 = minZ;
	m_aabbMax[ 0 ]	 = maxX;
	m_aabbMax[ 1 ]	 = maxY;
	m_aabbMax[ 2 ]	 = maxZ;
	m_baseVertex	 = 0;
	m_isLeafNode	 = false;
}

QuadtreeNode::QuadtreeNode( const float minX, const float maxX,
							const float minY, const float maxY,
							const float minZ, const float maxZ,
							const unsigned int baseVertex )
{
	m_pChildren[ 0 ] = NULL;
	m_pChildren[ 1 ] = NULL;
	m_pChildren[ 2 ] = NULL;
	m_pChildren[ 3 ] = NULL;
	m_aabbMin[ 0 ]	 = minX;
	m_aabbMin[ 1 ]	 = minY;
	m_aabbMin[ 2 ]	 = minZ;
	m_aabbMax[ 0 ]	 = maxX;
	m_aabbMax[ 1 ]	 = maxY;
	m_aabbMax[ 2 ]	 = maxZ;
	m_baseVertex	 = baseVertex;
	m_isLeafNode	 = true;
}

//------------------------------------------------------------------------------
// Name: ~QuadtreeNode()
// Desc: Destructor for the quadtree node class
//------------------------------------------------------------------------------
QuadtreeNode::~QuadtreeNode()
{
	//delete the child nodes
	delete m_pChildren[ 0 ];
	delete m_pChildren[ 1 ];
	delete m_pChildren[ 2 ];
	delete m_pChildren[ 3 ];
}

//------------------------------------------------------------------------------
// Name: AddVisibleNodes()
// Desc: Finds all visible nodes in this branch and the start vertex number to a
//		 list
//------------------------------------------------------------------------------
void QuadtreeNode::AddVisibleNodes( const Frustum& frustum,
									std::vector<unsigned int>& nodeList ) const
{
	//test against frustum
	INTERSECTION_RESULT ir = IntersectFrustum( frustum );

	if( ir == INSIDE )
	{
		//all child nodes must therefore be inside
		AddAllNodes( nodeList );
	}
	else if( ir == INTERSECTING )
	{
		//intersecting with frustum boundary
		//see if we have children to parse
		if( m_isLeafNode )
		{
			//add this node to the list
			nodeList.push_back( m_baseVertex );
			return;
		}
		else
		{
			//test each child
			m_pChildren[ 0 ]->AddVisibleNodes( frustum, nodeList );
			m_pChildren[ 1 ]->AddVisibleNodes( frustum, nodeList );
			m_pChildren[ 2 ]->AddVisibleNodes( frustum, nodeList );
			m_pChildren[ 3 ]->AddVisibleNodes( frustum, nodeList );
			return;
		}
	}
	//if we get this far, all child nodes must be outside
}

//------------------------------------------------------------------------------
// Name: AddAllNodes()
// Desc: Adds start vertex numbers for all nodes in this branch to a list
//------------------------------------------------------------------------------
void QuadtreeNode::AddAllNodes( std::vector<unsigned int>& nodeList ) const
{
	if( m_isLeafNode )
	{
        //add this node to the list
		nodeList.push_back( m_baseVertex );
		return;
	}
	else
	{
		//add each child
		m_pChildren[ 0 ]->AddAllNodes( nodeList );
		m_pChildren[ 1 ]->AddAllNodes( nodeList );
		m_pChildren[ 2 ]->AddAllNodes( nodeList );
		m_pChildren[ 3 ]->AddAllNodes( nodeList );
		return;
	}
}

//------------------------------------------------------------------------------
// Name: IntersectFrustum()
// Desc: Tests to see if the node is inside/outside/intersecting a frustum
//------------------------------------------------------------------------------
QuadtreeNode::INTERSECTION_RESULT QuadtreeNode::IntersectFrustum( const Frustum& frustum ) const
{
	bool intersecting = false;
	D3DXVECTOR3 vMin, vMax, n;

	//for each plane in the frustum
	for( int planeNum = 0; planeNum < 6; ++planeNum )
	{
		n = D3DXVECTOR3( frustum.planes[ planeNum ].a,
						 frustum.planes[ planeNum ].b,
						 frustum.planes[ planeNum ].c );

		//for each dimension
		for( int dim = 0; dim < 3; ++dim )
		{
			//calculate the two candidate points
			if( n[ dim ] >= 0 )
			{
				vMin[ dim ] = m_aabbMin[ dim ];
				vMax[ dim ] = m_aabbMax[ dim ];
			}
			else
			{
				vMin[ dim ] = m_aabbMax[ dim ];
				vMax[ dim ] = m_aabbMin[ dim ];
			}
		}

		//test which side of the plane each point is on
		if( ( D3DXVec3Dot( &n, &vMin ) + frustum.planes[ planeNum ].d ) > 0 )
			return OUTSIDE;

		if( ( D3DXVec3Dot( &n, &vMax ) + frustum.planes[ planeNum ].d ) >= 0 )
			intersecting = true;
	}

	if( intersecting )
		return INTERSECTING;
	else
		return INSIDE;
}