/* Copyright (c) <2003-2022> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __ND_SHAPE_H__ 
#define __ND_SHAPE_H__ 

#include "ndCollisionStdafx.h"

class ndBody;
class ndShape;
class ndShapeBox;
class ndShapeNull;
class ndShapeCone;
class ndShapePoint;
class ndShapeConvex;
class ndShapeSphere;
class ndShapeCapsule;
class ndContactPoint;
class ndShapeCompound;
class ndShapeCylinder;
class ndShapeCompound;
class ndRayCastNotify;
class ndShapeInstance;
class ndShapeStatic_bvh;
class ndShapeStaticMesh;
class ndShapeHeightfield;
class ndShapeConvexPolygon;
class ndShapeDebugNotify;
class ndShapeChamferCylinder;
class ndShapeStaticProceduralMesh;


enum ndShapeID
{
	// do not change the order of these enum
	m_box = 0,
	m_cone,
	m_sphere,
	m_capsule,
	m_cylinder,
	m_chamferCylinder,
	m_convexHull, // this must be the last convex shape ID

	// special and non convex collisions.
	m_compound,
	m_staticMesh,
	m_heightField,
	m_staticProceduralMesh,
	m_nullCollision,
	m_pointCollision,
	m_polygonCollision,
	m_boundingBoxHierachy,
	
	//m_deformableClothPatch,
	//m_deformableSolidMesh,
	// these are for internal use only	
	//m_contactCloud,
	//m_lumpedMassCollision
};

class ndShapeMaterial
{
	public:
	ndShapeMaterial()
		:m_userId(0)
	{
		m_data.m_alignPad = 0;
		for (ndInt32 i = 0; i < sizeof(m_userParam) / sizeof(m_userParam[0]); ++i)
		{
			m_userParam[i].m_intData = 0;
		}
	}

	ndInt64 m_userId;
	union nData
	{
		void* m_userData;
		ndUnsigned64 m_alignPad;
	} m_data;

	union dExtraData
	{
		void* m_ptrData;
		ndFloat32 m_floatData;
		ndUnsigned64 m_intData;
	} m_userParam[6];
};

struct ndBoxInfo
{
	ndFloat32 m_x;
	ndFloat32 m_y;
	ndFloat32 m_z;
};

struct ndPointInfo
{
	ndFloat32 m_noUsed;
};

struct ndSphereInfo
{
	ndFloat32 m_radius;
};

struct ndCylinderInfo
{
	ndFloat32 m_radio0;
	ndFloat32 m_radio1;
	ndFloat32 m_height;
};

struct ndCapsuleInfo
{
	ndFloat32 m_radio0;
	ndFloat32 m_radio1;
	ndFloat32 m_height;
};

struct ndConeInfo
{
	ndFloat32 m_radius;
	ndFloat32 m_height;
};

struct ndChamferCylinderInfo
{
	ndFloat32 m_r;
	ndFloat32 m_height;
};

struct ndConvexHullInfo
{
	ndInt32 m_vertexCount;
	ndInt32 m_strideInBytes;
	ndInt32 m_faceCount;
	ndVector* m_vertex;
};

struct ndCompoundInfo
{
	ndInt32 m_noUsed;
};

struct ndProceduralInfo
{
	ndInt32 m_noUsed;
};

struct ndCollisionBvhInfo
{
	ndInt32 m_vertexCount;
	ndInt32 m_indexCount;
};

struct ndHeighfieldInfo
{
	ndInt32 m_width;
	ndInt32 m_height;
	ndInt32 m_gridsDiagonals;
	ndFloat32 m_verticalScale;
	ndFloat32 m_horizonalScale_x;
	ndFloat32 m_horizonalScale_z;
	ndReal* m_elevation;
	ndInt8* m_atributes;
};

D_MSV_NEWTON_ALIGN_32
class ndShapeInfo
{
	public:
	ndShapeInfo();
	ndUnsigned64 GetHash(ndUnsigned64 hash);

	ndMatrix m_offsetMatrix;
	ndVector m_scale;
	ndShapeMaterial m_shapeMaterial;
	ndShapeID m_collisionType;
	union
	{
		ndBoxInfo m_box;
		ndConeInfo m_cone;
		ndPointInfo m_point;
		ndSphereInfo m_sphere;
		ndCapsuleInfo m_capsule;
		ndCollisionBvhInfo m_bvh;
		ndCylinderInfo m_cylinder;
		ndCompoundInfo m_compound;
		ndConvexHullInfo m_convexhull;
		ndHeighfieldInfo m_heightfield;
		ndProceduralInfo m_procedural;
		ndChamferCylinderInfo m_chamferCylinder;
		ndFloat32 m_paramArray[32];
	};

} D_GCC_NEWTON_ALIGN_32;

D_MSV_NEWTON_ALIGN_32
class ndShape: public ndContainersFreeListAlloc<ndShape>
{
	public:
	D_BASE_CLASS_REFLECTION(ndShape)
	D_COLLISION_API virtual ~ndShape();

	D_COLLISION_API ndInt32 GetRefCount() const;
	D_COLLISION_API virtual ndInt32 Release() const;
	D_COLLISION_API virtual const ndShape* AddRef() const;

	virtual ndShapeBox* GetAsShapeBox() { return nullptr; }
	virtual ndShapeNull* GetAsShapeNull() { return nullptr; }
	virtual ndShapeCone* GetAsShapeCone() { return nullptr; }
	virtual ndShapePoint* GetAsShapePoint() { return nullptr; }
	virtual ndShapeConvex* GetAsShapeConvex() { return nullptr; }
	virtual ndShapeSphere* GetAsShapeSphere() { return nullptr; }
	virtual ndShapeCapsule* GetAsShapeCapsule() { return nullptr; }
	virtual ndShapeCylinder* GetAsShapeCylinder() { return nullptr; }
	virtual ndShapeCompound* GetAsShapeCompound() { return nullptr; }
	virtual ndShapeStatic_bvh* GetAsShapeStaticBVH() { return nullptr; }
	virtual ndShapeStaticMesh* GetAsShapeStaticMesh() { return nullptr; }
	virtual ndShapeHeightfield* GetAsShapeHeightfield() { return nullptr; }
	virtual ndShapeConvexPolygon* GetAsShapeAsConvexPolygon() { return nullptr; }
	virtual ndShapeChamferCylinder* GetAsShapeChamferCylinder() { return nullptr; }
	virtual ndShapeStaticProceduralMesh* GetAsShapeStaticProceduralMesh() { return nullptr; }

	virtual ndInt32 GetConvexVertexCount() const;

	ndVector GetObbSize() const;
	ndVector GetObbOrigin() const;
	ndFloat32 GetUmbraClipSize() const;

	D_COLLISION_API virtual void MassProperties();

	virtual void DebugShape(const ndMatrix& matrix, ndShapeDebugNotify& debugCallback) const = 0;

	virtual ndShapeInfo GetShapeInfo() const;
	virtual ndFloat32 GetVolume() const = 0;
	virtual ndFloat32 GetBoxMinRadius() const = 0;
	virtual ndFloat32 GetBoxMaxRadius() const = 0;
	virtual ndUnsigned64 GetHash(ndUnsigned64 hash = 0) const;

	virtual void CalculateAabb(const ndMatrix& matrix, ndVector& p0, ndVector& p1) const = 0;
	virtual ndVector SupportVertex(const ndVector& dir, ndInt32* const vertexIndex) const = 0;
	virtual ndVector SupportVertexSpecialProjectPoint(const ndVector& point, const ndVector& dir) const = 0;
	virtual ndVector SupportVertexSpecial(const ndVector& dir, ndFloat32 skinMargin, ndInt32* const vertexIndex) const = 0;
	virtual ndInt32 CalculatePlaneIntersection(const ndVector& normal, const ndVector& point, ndVector* const contactsOut) const = 0;
	virtual ndVector CalculateVolumeIntegral(const ndMatrix& globalMatrix, const ndVector& globalPlane, const ndShapeInstance& parentScale) const = 0;
	virtual ndFloat32 RayCast(ndRayCastNotify& callback, const ndVector& localP0, const ndVector& localP1, ndFloat32 maxT, const ndBody* const body, ndContactPoint& contactOut) const = 0;

	virtual ndMatrix CalculateInertiaAndCenterOfMass(const ndMatrix& alignMatrix, const ndVector& localScale, const ndMatrix& matrix) const;
	virtual ndFloat32 CalculateMassProperties(const ndMatrix& offset, ndVector& inertia, ndVector& crossInertia, ndVector& centerOfMass) const;

	protected:
	D_COLLISION_API ndShape(ndShapeID id);
	D_COLLISION_API ndShape (const ndShape& source);

	ndVector m_inertia;	
	ndVector m_crossInertia;	
	ndVector m_centerOfMass;
	ndVector m_boxSize;
	ndVector m_boxOrigin;
	mutable ndAtomic<ndInt32> m_refCount;
	ndShapeID m_collisionId;
	static ndVector m_flushZero;
	friend class ndFileFormatShape;
} D_GCC_NEWTON_ALIGN_32;

inline ndShapeInfo::ndShapeInfo()
	:m_offsetMatrix(ndGetIdentityMatrix())
	,m_scale(ndFloat32 (1.0f))
	,m_shapeMaterial()
	,m_collisionType(::m_box)
{
	for (ndInt32 i = 0; i < sizeof(m_paramArray) / sizeof(m_paramArray[0]); ++i)
	{
		m_paramArray[i] = ndFloat32(0);
	}
}

inline ndUnsigned64 ndShapeInfo::GetHash(ndUnsigned64 hash)
{
	ndInt32 id = m_collisionType;
	hash = dCRC64(m_paramArray, sizeof (m_paramArray), hash);
	hash = dCRC64(&id, sizeof(id), hash);
	return hash;
}

inline ndInt32 ndShape::GetConvexVertexCount() const
{
	return 0;
}

inline ndFloat32 ndShape::CalculateMassProperties(const ndMatrix&, ndVector&, ndVector&, ndVector&) const
{ 
	ndAssert(0); 
	return 0; 
}

inline ndMatrix ndShape::CalculateInertiaAndCenterOfMass(const ndMatrix&, const ndVector&, const ndMatrix&) const
{
	ndAssert(0);
	return ndGetZeroMatrix();
}

inline ndVector ndShape::GetObbOrigin() const
{
	return m_boxOrigin;
}

inline ndVector ndShape::GetObbSize() const
{
	return m_boxSize;
}

inline ndFloat32 ndShape::GetUmbraClipSize() const
{
	return ndFloat32(3.0f) * GetBoxMaxRadius();
}

inline ndUnsigned64 ndShape::GetHash(ndUnsigned64 hash) const
{
	ndAssert(0);
	return hash;
}

#endif 


