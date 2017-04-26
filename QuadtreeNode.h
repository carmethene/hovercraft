//------------------------------------------------------------------------------
// File: QuadtreeNode.h
// Desc: A quadtree node for terrain that can be recursively tested against
//		 the view frustum and rendered if visible
//
// Created: 03 January 2003 08:27:35
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_QUADTREENODE_H
#define INCLUSIONGUARD_QUADTREENODE_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>
#include <vector>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
struct Frustum;

//------------------------------------------------------------------------------
// Name: class QuadtreeNode
// Desc: A node in the terrain quadtree
//------------------------------------------------------------------------------
class QuadtreeNode
{
public:
	const static int LEAFNODE_WIDTH = 40;	//width in quads

	QuadtreeNode();
	QuadtreeNode( QuadtreeNode* pChild1, QuadtreeNode* pChild2,
				  QuadtreeNode* pChild3, QuadtreeNode* pChild4,
				  const float minX, const float maxX,
				  const float minY, const float maxY,
				  const float minZ, const float maxZ );
    
	QuadtreeNode( const float minX, const float maxX,
				  const float minY, const float maxY,
				  const float minZ, const float maxZ,
				  const unsigned int baseVertex );

	~QuadtreeNode();

	inline float GetAABBMin( const int dim ) const { return m_aabbMin[ dim ]; }
	inline float GetAABBMax( const int dim ) const { return m_aabbMax[ dim ]; }

	void AddVisibleNodes( const Frustum& frustum, std::vector<unsigned int>& nodeList ) const;
	void AddAllNodes( std::vector<unsigned int>& nodeList ) const;
	
private:
	const static enum INTERSECTION_RESULT { OUTSIDE, INSIDE, INTERSECTING };

	INTERSECTION_RESULT IntersectFrustum( const Frustum& frustum ) const;

	QuadtreeNode* m_pChildren[ 4 ];
	unsigned int m_baseVertex;
	bool m_isLeafNode;

	//AABB
	float m_aabbMin[ 3 ];
	float m_aabbMax[ 3 ];

};

#endif //INCLUSIONGUARD_QUADTREENODE_H
