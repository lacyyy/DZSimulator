#ifndef COLL_COLLIDABLEWORLD_DISPLACEMENT_H_
#define COLL_COLLIDABLEWORLD_DISPLACEMENT_H_

#include <cassert>
#include <cstdint>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "coll/SweptTrace.h"
#include "csgo_parsing/BspMap.h"

// @OPTIMIZATION Test impact of __forceinline
#if defined( _WIN32 )
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE
#endif

namespace coll {

// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/dispcoll_common.h)

// Assumptions:
//    Max patch is 17x17, therefore 9 bits needed to represent a triangle index

// Displacement Collision Triangle
class CDispCollTri
{
    struct index_t
    {
        union
        {
            struct
            {
                unsigned short uiVert : 9;
                unsigned short uiMin  : 2;
                unsigned short uiMax  : 2;
            } m_Index;

            unsigned short m_IndexDummy;
        };
    };

    index_t         m_TriData[3];

public:
    unsigned short  m_ucSignBits  : 3; // Plane test.
    unsigned short  m_ucPlaneType : 3; // Axial test?

    Magnum::Vector3 m_vecNormal; // Triangle normal (plane normal).
    float           m_flDist;    // Triangle plane dist.

    // Creation.
    CDispCollTri();
    void Init();
    void CalcPlane (std::vector<Magnum::Vector3>& m_aVerts);
    void FindMinMax(std::vector<Magnum::Vector3>& m_aVerts);

    // Triangle data.
    inline void SetVert(int iPos, int iVert) { assert((iPos  >= 0) && (iPos  < 3)); assert((iVert >= 0) && (iVert < (1 << 9))); m_TriData[iPos].m_Index.uiVert = iVert; }
    inline int  GetVert(int iPos) const      { assert((iPos  >= 0) && (iPos  < 3)); return m_TriData[iPos].m_Index.uiVert; }
    inline void SetMin(int iAxis, int iMin)  { assert((iAxis >= 0) && (iAxis < 3)); assert((iMin >= 0) && (iMin < 3)); m_TriData[iAxis].m_Index.uiMin = iMin; }
    inline int  GetMin(int iAxis) const      { assert((iAxis >= 0) && (iAxis < 3)); return m_TriData[iAxis].m_Index.uiMin; }
    inline void SetMax(int iAxis, int iMax)  { assert((iAxis >= 0) && (iAxis < 3)); assert((iMax >= 0) && (iMax < 3)); m_TriData[iAxis].m_Index.uiMax = iMax; }
    inline int  GetMax(int iAxis) const      { assert((iAxis >= 0) && (iAxis < 3)); return m_TriData[iAxis].m_Index.uiMax; }
};

// Helper
class CDispCollHelper
{
public:
    float            m_flStartFrac;
    float            m_flEndFrac;
    Magnum::Vector3  m_vecImpactNormal;
    float            m_flImpactDist;
};

// Cache
#pragma pack(1)
class CDispCollTriCache
{
public:
    unsigned short m_iCrossX[3];
    unsigned short m_iCrossY[3];
    unsigned short m_iCrossZ[3];
};
#pragma pack()

class CDispCollNode
{
public:
    // ==== The following is the original source-sdk-2013 code that utilizes SIMD.
    ////FourVectors m_mins;
    ////FourVectors m_maxs;
    
    // ==== The following is the non-SIMD replacement of the code above.
    // AABBs of all 4 node children, index 0=SW, 1=SE, 2=NW, 3=NE
    Magnum::Vector3 m_mins[4];
    Magnum::Vector3 m_maxs[4];
};

class CDispCollLeaf
{
public:
    short m_tris[2];
};

// A power 4 displacement can have 341 nodes, pad out to 344 for 16-byte alignment
const int MAX_DISP_AABB_NODES = 341;
const int MAX_AABB_LIST       = 344;

struct rayleaflist_t
{
    // ==== The following is the original source-sdk-2013 code that utilizes SIMD.
    ////FourVectors rayStart;
    ////FourVectors rayExtents;
    ////FourVectors invDelta;
    // ==== The following is the non-SIMD replacement of the code above.
    Magnum::Vector3 rayStart;
    Magnum::Vector3 rayExtents;
    Magnum::Vector3 invDelta;
    // ==== end of replacement

    int nodeList[MAX_AABB_LIST];
    int maxIndex;
};

// Displacement Collision Tree Data
class CDispCollTree
{
public:
    // Creation. Takes index of displacement and the BspMap object containing it.
    CDispCollTree(size_t disp_info_idx, const csgo_parsing::BspMap& bsp_map);

    // Raycasts. DOES NOT utilize collision caches.
    // Does nothing and returns false if displacement has NO_RAY_COLL flag set.
    bool AABBTree_Ray(SweptTrace* trace, bool bSide = true);

    // Hull Sweeps. DOES utilize collision caches and might create one.
    // Does nothing and returns false if displacement has NO_HULL_COLL flag set.
    // CAUTION: Not thread-safe yet! (Due to unprotected g_DispCollPlaneIndexHash)
    bool AABBTree_SweepAABB(SweptTrace* trace);

    // Hull Intersection. DOES NOT utilize collision caches.
    // Does nothing and returns false if displacement has NO_HULL_COLL flag set.
    bool AABBTree_IntersectAABB(
        const Magnum::Vector3& absMins,
        const Magnum::Vector3& absMaxs);

    // Utility.
    inline int  GetFlags()             const { return m_nFlags; }
    inline bool CheckFlags(int nFlags) const { return ((nFlags & GetFlags()) != 0) ? true : false; }

    inline int GetWidth()   const { return ((1 << m_nPower) + 1); }
    inline int GetHeight()  const { return ((1 << m_nPower) + 1); }
    inline int GetSize()    const { return ((1 << m_nPower) + 1) * ((1 << m_nPower) + 1); }
    inline int GetTriSize() const { return ((1 << m_nPower) * (1 << m_nPower) * 2); }

    inline void GetBounds(Magnum::Vector3& vecBoxMin, Magnum::Vector3& vecBoxMax) const { vecBoxMin = m_mins; vecBoxMax = m_maxs; }

public:
    inline int Nodes_GetChild(int iNode, int nDirection) const;
    inline int Nodes_CalcCount(int nPower) const;
    inline int Nodes_GetIndexFromComponents(int x, int y) const;

    bool IsCacheGenerated() const;
    void EnsureCacheIsCreated();
    void Uncache();

private:
    void AABBTree_Create      (const std::vector<Magnum::Vector3>& disp_vertices);
    void AABBTree_CopyDispData(const std::vector<Magnum::Vector3>& disp_vertices);
    void AABBTree_CreateLeafs();
    void AABBTree_GenerateBoxes_r(int nodeIndex, Magnum::Vector3* pMins, Magnum::Vector3* pMaxs);
    void AABBTree_CalcBounds();

    void AABBTree_TreeTrisRayTest(SweptTrace* trace, int iNode, bool bSide, CDispCollTri** pImpactTri);
    
    int FORCEINLINE BuildRayLeafList(int iNode, rayleaflist_t& list);

private:
    void SweepAABBTriIntersect(SweptTrace* trace, int iTri, CDispCollTri* pTri);

    void Cache_Create(CDispCollTri* pTri, int iTri);
    bool Cache_EdgeCrossAxisX(const Magnum::Vector3& vecEdge, const Magnum::Vector3& vecOnEdge, const Magnum::Vector3& vecOffEdge, CDispCollTri* pTri, unsigned short& iPlane);
    bool Cache_EdgeCrossAxisY(const Magnum::Vector3& vecEdge, const Magnum::Vector3& vecOnEdge, const Magnum::Vector3& vecOffEdge, CDispCollTri* pTri, unsigned short& iPlane);
    bool Cache_EdgeCrossAxisZ(const Magnum::Vector3& vecEdge, const Magnum::Vector3& vecOnEdge, const Magnum::Vector3& vecOffEdge, CDispCollTri* pTri, unsigned short& iPlane);

    inline bool FacePlane(const SweptTrace& trace, CDispCollTri* pTri, CDispCollHelper* pHelper);
    bool FORCEINLINE AxisPlanesXYZ(const SweptTrace& trace, CDispCollTri* pTri, CDispCollHelper* pHelper);
    inline bool EdgeCrossAxisX(const SweptTrace& trace, unsigned short iPlane, CDispCollHelper* pHelper);
    inline bool EdgeCrossAxisY(const SweptTrace& trace, unsigned short iPlane, CDispCollHelper* pHelper);
    inline bool EdgeCrossAxisZ(const SweptTrace& trace, unsigned short iPlane, CDispCollHelper* pHelper);

    bool ResolveRayPlaneIntersect(float flStart, float flEnd, const Magnum::Vector3& vecNormal, float flDist, CDispCollHelper* pHelper);
    template <int AXIS> bool EdgeCrossAxis(const SweptTrace& trace, unsigned short iPlane, CDispCollHelper* pHelper);

    // Utility
    inline void CalcClosestExtents(const Magnum::Vector3& vecPlaneNormal, const Magnum::Vector3& vecBoxExtents, Magnum::Vector3& vecBoxPoint);
    int AddPlane(const Magnum::Vector3& vecNormal);
    bool FORCEINLINE IsLeafNode(int iNode);

public:
    // Bounding box of the displacement surface, slightly bloated
    Magnum::Vector3 m_mins;
    Magnum::Vector3 m_maxs;

private:
    int m_nPower; // Size of the displacement ( 2^power + 1 )
    int m_nFlags;

private:
    std::vector<Magnum::Vector3>   m_aVerts; // Displacement verts.
    std::vector<CDispCollTri>      m_aTris;  // Displacement triangles.
    std::vector<CDispCollNode>     m_nodes;  // Nodes.
    std::vector<CDispCollLeaf>     m_leaves; // Leaves.

    // Collision cache, created and destroyed by EnsureCacheIsCreated() and Uncache()
    std::vector<CDispCollTriCache> m_aTrisCache;
    std::vector<Magnum::Vector3>   m_aEdgePlanes;

private:
    // Debugger needs to debug, let it access private members.
    friend class Debugger;
};

// Purpose: get the child node index given the current node index and direction
//          of the child (1 of 4)
//   Input: iNode - current node index
//          nDirection - direction of the child ( [0...3] - SW, SE, NW, NE )
//  Output: int - the index of the child node
inline int CDispCollTree::Nodes_GetChild(int iNode, int nDirection) const {
    // node range [0...m_NodeCount)
    assert(iNode >= 0);
    assert(iNode < m_nodes.size());
    // ( node index * 4 ) + ( direction + 1 )
    return ((iNode << 2) + (nDirection + 1));
}

inline int CDispCollTree::Nodes_CalcCount(int nPower) const {
    assert(nPower >= 1);
    assert(nPower <= 4);
    return ((1 << ((nPower + 1) << 1)) / 3);
}

inline int CDispCollTree::Nodes_GetIndexFromComponents(int x, int y) const {
    int nIndex = 0;

    // Interleave bits from the x and y values to create the index
    for (int iShift = 0; x != 0; iShift += 2, x >>= 1)
        nIndex |= (x & 1) << iShift;

    for (int iShift = 1; y != 0; iShift += 2, y >>= 1)
        nIndex |= (y & 1) << iShift;

    return nIndex;
}
// --------- end of source-sdk-2013 code ---------

} // namespace coll

#endif // COLL_COLLIDABLEWORLD_DISPLACEMENT_H_
