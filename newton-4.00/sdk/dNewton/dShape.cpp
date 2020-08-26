/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
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

#include "dNewtonStdafx.h"
#include "dBody.h"
#include "dShape.h"
#include "dNewton.h"

//#include "dgPhysicsStdafx.h"
//#include "dgBody.h"
//#include "dgWorld.h"
//#include "dShape.h"


dVector dShape::m_flushZero(dFloat32(1.0e-7f));

#if 0

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dShape::dShape (dgWorld* const world, dgDeserialize deserialization, void* const userData, dInt32 revision)
	:m_inertia(dVector::m_zero)
	,m_crossInertia(dVector::m_zero)
	,m_centerOfMass(dVector::m_zero)
	,m_boxSize (dVector::m_zero)
	,m_boxOrigin (dVector::m_zero)
	,m_rtti(0)
	,m_refCount(1)
	,m_signature(0)
	,m_collisionId(dShapeID(0))
	,m_allocator(world->GetAllocator())
{
	dInt32 collisionId;
	deserialization (userData, &m_inertia, sizeof (m_inertia));
	deserialization (userData, &m_crossInertia, sizeof (m_crossInertia));
	deserialization (userData, &m_centerOfMass, sizeof (m_centerOfMass));
	deserialization (userData, &m_boxSize, sizeof (m_boxSize));
	deserialization (userData, &m_boxOrigin, sizeof (m_boxOrigin));
	deserialization (userData, &m_rtti, sizeof (m_rtti));
	deserialization (userData, &m_signature, sizeof (m_signature));
	deserialization (userData, &collisionId, sizeof (collisionId));
	m_collisionId = dShapeID(collisionId);
}


dUnsigned32 dShape::Quantize(dFloat32 value)
{
	return dUnsigned32 (value * 1024.0f);
}

dUnsigned32 dShape::Quantize(const void* const buffer, int size)
{
	dUnsigned32 crc = dgCRC (buffer, size);
	return crc;
}

void dShape::GetCollisionInfo(dShapeInfo* const info) const
{
	info->m_collisionType = m_collisionId;
}

void dShape::SerializeLow (dgSerialize callback, void* const userData) const
{
	dInt32 collisionId = m_collisionId;
	callback (userData, &m_inertia, sizeof (m_inertia));
	callback (userData, &m_crossInertia, sizeof (m_crossInertia));
	callback (userData, &m_centerOfMass, sizeof (m_centerOfMass));
	callback (userData, &m_boxSize, sizeof (m_boxSize));
	callback (userData, &m_boxOrigin, sizeof (m_boxOrigin));
	callback (userData, &m_rtti, sizeof (m_rtti));
	callback (userData, &m_signature, sizeof (m_signature));
	callback (userData, &collisionId, sizeof (collisionId));
}
#endif


dShape::dShape(dShapeID id)
	:dClassAlloc()
	,m_inertia(dVector::m_zero)
	,m_crossInertia(dVector::m_zero)
	,m_centerOfMass(dVector::m_zero)
	,m_boxSize(dVector::m_zero)
	,m_boxOrigin(dVector::m_zero)
	,m_refCount(0)
//	,m_signature(signature)
//	,m_collisionId(id)
//	,m_allocator(allocator)
{
}

dShape::dShape(const dShape& source)
	:dClassAlloc()
	,m_inertia(source.m_inertia)
	,m_crossInertia(source.m_crossInertia)
	,m_centerOfMass(source.m_centerOfMass)
	,m_boxSize(source.m_boxSize)
	,m_boxOrigin(source.m_boxOrigin)
	,m_refCount(0)
	//,m_signature(source.m_signature)
	//,m_collisionId(source.m_collisionId)
	//,m_allocator(source.m_allocator)
{
}

dShape::~dShape()
{
	dAssert(m_refCount.load() == 0);
}

void dShape::MassProperties()
{
	// using general central theorem, to extract the Inertia relative to the center of mass 
	//IImatrix = IIorigin + unitmass * [(displacemnet % displacemnet) * identityMatrix - transpose(displacement) * displacement)];

	dMatrix inertia(dGetIdentityMatrix());
	inertia[0][0] = m_inertia[0];
	inertia[1][1] = m_inertia[1];
	inertia[2][2] = m_inertia[2];
	inertia[0][1] = m_crossInertia[2];
	inertia[1][0] = m_crossInertia[2];
	inertia[0][2] = m_crossInertia[1];
	inertia[2][0] = m_crossInertia[1];
	inertia[1][2] = m_crossInertia[0];
	inertia[2][1] = m_crossInertia[0];

	dVector origin(m_centerOfMass);
	dFloat32 originMag2 = origin.DotProduct(origin & dVector::m_triplexMask).GetScalar();

	dMatrix Covariance(origin, origin);
	dMatrix parallel(dGetIdentityMatrix());
	for (dInt32 i = 0; i < 3; i++) {
		parallel[i][i] = originMag2;
		inertia[i] += (parallel[i] - Covariance[i]);
		dAssert(inertia[i][i] > dFloat32(0.0f));
	}

	m_inertia[0] = inertia[0][0];
	m_inertia[1] = inertia[1][1];
	m_inertia[2] = inertia[2][2];
	m_crossInertia[0] = inertia[2][1];
	m_crossInertia[1] = inertia[2][0];
	m_crossInertia[2] = inertia[1][0];
}
