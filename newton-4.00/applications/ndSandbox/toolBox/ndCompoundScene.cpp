/* Copyright (c) <2003-2021> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "ndSandboxStdafx.h"
#include "ndDemoMesh.h"
#include "ndDemoEntity.h"
#include "ndPhysicsWorld.h"
#include "ndCompoundScene.h"
#include "ndDemoEntityManager.h"
#include "ndHeightFieldPrimitive.h"

static void AddBoxSubShape(ndDemoEntityManager* const scene, ndShapeInstance& sceneInstance, ndDemoEntity* const rootEntity, const dMatrix& location)
{
	ndShapeInstance box(new ndShapeBox(200.0f, 1.0f, 200.f));
	dMatrix uvMatrix(dGetIdentityMatrix());
	uvMatrix[0][0] *= 0.025f;
	uvMatrix[1][1] *= 0.025f;
	uvMatrix[2][2] *= 0.025f;
	ndDemoMesh* const geometry = new ndDemoMesh("box", scene->GetShaderCache(), &box, "marbleCheckBoard.tga", "marbleCheckBoard.tga", "marbleCheckBoard.tga", 1.0f, uvMatrix);

	dMatrix matrix(dGetIdentityMatrix());
	matrix.m_posit.m_y = -0.5f;
	ndDemoEntity* const entity = new ndDemoEntity(matrix, rootEntity);
	entity->SetMesh(geometry, location);
	geometry->Release();

	ndShapeMaterial material(box.GetMaterial());
	material.m_userData = entity;
	box.SetMaterial(material);

	box.SetLocalMatrix(location);
	ndShapeCompound* const compound = sceneInstance.GetShape()->GetAsShapeCompound();
	compound->AddCollision(&box);

	//ndShapeCompound::ndTreeArray::dNode* const node = compound->AddCollision(&heighfieldInstance);
	//ndShapeInstance* const subInstance = node->GetInfo()->GetShape();
	//return subInstance;
}

ndBodyKinematic* BuildCompoundScene(ndDemoEntityManager* const scene, const dMatrix& location)
{
	ndDemoEntity* const rootEntity = new ndDemoEntity(location, nullptr);
	scene->AddEntity(rootEntity);

	ndShapeInstance sceneInstance (new ndShapeCompound());
	ndShapeCompound* const compound = sceneInstance.GetShape()->GetAsShapeCompound();
	compound->BeginAddRemove();

	dMatrix boxLocation(location);
	boxLocation.m_posit = boxLocation.m_posit.Scale(-1.0f);
	boxLocation.m_posit.m_w = 1.0f;
	AddBoxSubShape(scene, sceneInstance, rootEntity, boxLocation);

	//boxLocation.m_posit.m_y -= 1.0f;
	//AddBoxSubShape(scene, sceneInstance, rootEntity, boxLocation);

	//AddHeightfieldSubShape(scene, sceneInstance, rootEntity);
	
	compound->EndAddRemove();

	ndPhysicsWorld* const world = scene->GetWorld();
	ndBodyDynamic* const body = new ndBodyDynamic();
	body->SetNotifyCallback(new ndDemoEntityNotify(scene, rootEntity));
	body->SetMatrix(location);
	body->SetCollisionShape(sceneInstance);

	world->AddBody(body);
	return body;
}