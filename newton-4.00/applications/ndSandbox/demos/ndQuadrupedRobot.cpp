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
#include "ndSkyBox.h"
#include "ndTargaToOpenGl.h"
#include "ndDemoMesh.h"
#include "ndDemoCamera.h"
#include "ndLoadFbxMesh.h"
#include "ndPhysicsUtils.h"
#include "ndPhysicsWorld.h"
#include "ndMakeStaticMap.h"
#include "ndDemoEntityManager.h"
#include "ndDemoInstanceEntity.h"

class dQuadrupedRobotDefinition
{
	public:
	enum jointType
	{
		m_root,
		m_hinge,
		m_spherical,
		m_effector
	};

	char m_boneName[32];
	jointType m_type;
	ndFloat32 m_mass;
	ndFloat32 m_walkPhase;
};

static dQuadrupedRobotDefinition jointsDefinition[] =
{
	{ "root_Bone010", dQuadrupedRobotDefinition::m_root, 40.0f},

	{ "rb_thigh_Bone014", dQuadrupedRobotDefinition::m_spherical, 3.0f },
	{ "rb_knee_Bone013", dQuadrupedRobotDefinition::m_hinge, 2.0f },
	{ "rb_effector_Bone009", dQuadrupedRobotDefinition::m_effector , 0.0f, 0.0f },
	
	{ "lb_thigh_Bone011", dQuadrupedRobotDefinition::m_spherical, 3.0f },
	{ "lb_knee_Bone012", dQuadrupedRobotDefinition::m_hinge, 2.0f },
	{ "lb_effector_Bone010", dQuadrupedRobotDefinition::m_effector , 0.0f, 0.5f },
	
	{ "fr_thigh_Bone003", dQuadrupedRobotDefinition::m_spherical, 3.0f },
	{ "fr_knee_Bone004", dQuadrupedRobotDefinition::m_hinge, 2.0f },
	{ "fr_effector_Bone005", dQuadrupedRobotDefinition::m_effector , 0.0f, 0.75f },
	
	{ "fl_thigh_Bone008", dQuadrupedRobotDefinition::m_spherical, 3.0f },
	{ "fl_knee_Bone006", dQuadrupedRobotDefinition::m_hinge, 2.0f },
	{ "fl_effector_Bone007", dQuadrupedRobotDefinition::m_effector , 0.0f, 0.25f },
};

class dQuadrupedRobot : public ndModel
{
	public:
	#define D_SAMPLES_COUNT 128

	D_CLASS_REFLECTION(dQuadrupedRobot);

	class dQuadrupedLeg : public ndIk6DofEffector
	{
		public:
		dQuadrupedLeg(const ndMatrix& pinAndPivotChild, const ndMatrix& pinAndPivotParent, ndBodyKinematic* const child, ndBodyKinematic* const parent, ndFloat32 phaseAngle, ndFloat32 swayAmp)
			:ndIk6DofEffector(pinAndPivotChild, pinAndPivotParent, child, parent)
			,m_walkPhaseAngle(phaseAngle)
			,m_footOnGround(0)
		{
			ndFloat32 regularizer = 1.0e-4f;
			EnableRotationAxis(ndIk6DofEffector::m_shortestPath);
			SetLinearSpringDamper(regularizer, 2500.0f, 50.0f);
			SetAngularSpringDamper(regularizer, 2500.0f, 50.0f);
			
			m_offset = GetOffsetMatrix().m_posit;

			// generate very crude animation. 
			GenerateSimpleWalkCycle(swayAmp);
		}

		void DebugJoint(ndConstraintDebugCallback& debugCallback) const
		{
			// TODO: maybe draw the trajectory here
			ndIk6DofEffector::DebugJoint(debugCallback);
		}

		void GenerateSimpleWalkCycle(ndFloat32 swayAmp)
		{
			// clear pose vector
			for (ndInt32 i = 0; i < (D_SAMPLES_COUNT + 1); i++)
			{
				m_walkCurve[i] = ndVector::m_zero;
			}

			const ndInt32 splitParam = D_SAMPLES_COUNT / 2 + 1;

			// vertical leg motion
			ndFloat32 amplitud = 0.1f;
			ndFloat32 period = ndPi / (D_SAMPLES_COUNT - splitParam - 2);
			for (ndInt32 i = splitParam; i < D_SAMPLES_COUNT; i++)
			{
				ndFloat32 j = ndFloat32(i - splitParam - 1);
				m_walkCurve[i].m_x = -amplitud * ndSin(period * j);
			}

			// horizontal motion
			ndFloat32 stride = 0.4f;
			for (ndInt32 i = 0; i < splitParam; i++)
			{
				ndFloat32 j = ndFloat32(i);
				m_walkCurve[i].m_y = stride  * (0.5f - j / splitParam);
			}

			for (ndInt32 i = splitParam; i < D_SAMPLES_COUNT; i++)
			{
				ndFloat32 j = ndFloat32(i - splitParam);
				m_walkCurve[i].m_y = -stride  * (0.5f - j / (D_SAMPLES_COUNT - splitParam));
			}
			m_walkCurve[D_SAMPLES_COUNT].m_y = m_walkCurve[0].m_y;

			// sideway motion
			for (ndInt32 i = 0; i < splitParam; i++)
			{
				m_walkCurve[i].m_z = swayAmp;
			}
			for (ndInt32 i = splitParam; i < D_SAMPLES_COUNT; i++)
			{
				ndFloat32 j = ndFloat32(i - splitParam - 1);
				m_walkCurve[i].m_z = swayAmp * (1.0f - ndSin(period * j));
			}
			m_walkCurve[D_SAMPLES_COUNT].m_z = m_walkCurve[0].m_z;
		}

		void GeneratePose(ndFloat32 parameter)
		{
			ndVector localPosit(m_offset);
			ndMatrix targetMatrix(dGetIdentityMatrix());

			ndInt32 index = ndInt32 ((parameter + m_walkPhaseAngle) * D_SAMPLES_COUNT) % D_SAMPLES_COUNT;
			localPosit.m_x += m_walkCurve[index].m_x;
			localPosit.m_y += m_walkCurve[index].m_y;
			localPosit.m_z += m_walkCurve[index].m_z;

			m_footOnGround = (index < D_SAMPLES_COUNT / 2 + 1);
			
			targetMatrix.m_posit = localPosit;
			SetOffsetMatrix(targetMatrix);
		}

		ndVector m_offset;
		ndFloat32 m_walkPhaseAngle;
		ndVector m_walkCurve[D_SAMPLES_COUNT + 1];

		ndInt32 m_footOnGround;
	};

	dQuadrupedRobot(ndDemoEntityManager* const scene, fbxDemoEntity* const robotMesh, const ndMatrix& location)
		:ndModel()
		,m_referenceFrame(dGetIdentityMatrix())
		,m_rootBody(nullptr)
		,m_effectors()
		,m_invDynamicsSolver()
		,m_bodyArray()
		,m_jointArray()
		,m_walkParam(0.0f)
	{
		// make a clone of the mesh and add it to the scene
		ndDemoEntity* const entity = (ndDemoEntity*)robotMesh->CreateClone();
		scene->AddEntity(entity);
		ndWorld* const world = scene->GetWorld();

		ndDemoEntity* const rootEntity = entity->Find(jointsDefinition[0].m_boneName);
	
		// find the floor location 
		ndMatrix matrix(rootEntity->CalculateGlobalMatrix() * location);
		ndVector floor(FindFloor(*world, matrix.m_posit + ndVector(0.0f, 100.0f, 0.0f, 0.0f), 200.0f));
		matrix.m_posit.m_y = floor.m_y;

		matrix.m_posit.m_y += 0.75f;
		rootEntity->ResetMatrix(matrix);

		// add the root body
		m_rootBody = CreateBodyPart(scene, rootEntity, jointsDefinition[0].m_mass, nullptr);
		m_bodyArray.PushBack(m_rootBody);
		m_referenceFrame = rootEntity->Find("referenceFrame")->CalculateGlobalMatrix(rootEntity);

		ndFixSizeArray<ndDemoEntity*, 32> childEntities;
		ndFixSizeArray<ndBodyDynamic*, 32> parentBone;

		ndInt32 stack = 0;
		for (ndDemoEntity* child = rootEntity->GetChild(); child; child = child->GetSibling())
		{
			childEntities[stack] = child;
			parentBone[stack] = m_rootBody;
			stack++;
		}

		const ndInt32 definitionCount = ndInt32 (sizeof(jointsDefinition) / sizeof(jointsDefinition[0]));
		while (stack) 
		{
			stack--;
			ndBodyDynamic* parentBody = parentBone[stack];
			ndDemoEntity* const childEntity = childEntities[stack];

			const char* const name = childEntity->GetName().GetStr();
			for (ndInt32 i = 0; i < definitionCount; i++) 
			{
				const dQuadrupedRobotDefinition& definition = jointsDefinition[i];
				if (!strcmp(definition.m_boneName, name))
				{
					//dTrace(("name: %s\n", name));
					if (definition.m_type == dQuadrupedRobotDefinition::m_hinge)
					{

						ndBodyDynamic* const childBody = CreateBodyPart(scene, childEntity, definition.m_mass, parentBody);
						m_bodyArray.PushBack(childBody);
						
						const ndMatrix pivotMatrix(dRollMatrix(90.0f * ndDegreeToRad) * childBody->GetMatrix());
						ndIkJointHinge* const hinge = new ndIkJointHinge(pivotMatrix, childBody, parentBody);
						hinge->SetLimits(-60.0f * ndDegreeToRad, 90.0f * ndDegreeToRad);
						m_jointArray.PushBack(hinge);
						world->AddJoint(hinge);
						parentBody = childBody;
					}
					else if (definition.m_type == dQuadrupedRobotDefinition::m_spherical)
					{
						ndBodyDynamic* const childBody = CreateBodyPart(scene, childEntity, definition.m_mass, parentBody);
						m_bodyArray.PushBack(childBody);
						
						const ndMatrix pivotMatrix(dYawMatrix(90.0f * ndDegreeToRad) * childBody->GetMatrix());
						ndIkJointSpherical* const socket = new ndIkJointSpherical(pivotMatrix, childBody, parentBody);
						socket->SetConeLimit(120.0f * ndDegreeToRad);
						socket->SetTwistLimits(-90.0f * ndDegreeToRad, 90.0f * ndDegreeToRad);

						world->AddJoint(socket);
						parentBody = childBody;
					}
					else
					{
						char refName[256];
						sprintf(refName, "%sreference", name);
						dAssert(rootEntity->Find(refName));

						const ndMatrix effectorFrame(childEntity->CalculateGlobalMatrix());
						const ndMatrix pivotFrame(rootEntity->Find(refName)->CalculateGlobalMatrix());

						ndShapeInstance sphere(new ndShapeSphere(0.05f));
						sphere.SetCollisionMode(false);

						ndBodyDynamic* const childBody = new ndBodyDynamic();
						childBody->SetMatrix(effectorFrame);
						childBody->SetCollisionShape(sphere);
						childBody->SetMassMatrix(1.0f / parentBody->GetInvMass(), sphere);
						childBody->SetNotifyCallback(new ndDemoEntityNotify(scene, nullptr, parentBody));
						m_bodyArray.PushBack(childBody);

						ndMatrix bootFrame;
						bootFrame.m_front = pivotFrame.m_up;
						bootFrame.m_up = pivotFrame.m_right;
						bootFrame.m_right = bootFrame.m_front.CrossProduct(bootFrame.m_up);
						bootFrame.m_posit = effectorFrame.m_posit;

						ndJointDoubleHinge* const bootJoint = new ndIkJointDoubleHinge(bootFrame, childBody, parentBody);
						bootJoint->SetLimits0(-90.0f * ndDegreeToRad, 90.0f * ndDegreeToRad);
						bootJoint->SetLimits1(-90.0f * ndDegreeToRad, 90.0f * ndDegreeToRad);
						// add body to the world
						scene->GetWorld()->AddBody(childBody);
						scene->GetWorld()->AddJoint(bootJoint);

						ndVector legSide(m_rootBody->GetMatrix().UntransformVector(pivotFrame.m_posit));
						ndFloat32 swayAmp(0.8f * legSide.m_y);

						dQuadrupedLeg* const effector = new dQuadrupedLeg(effectorFrame, pivotFrame, childBody, m_rootBody, definition.m_walkPhase, swayAmp);
						m_effectors.PushBack(effector);
					}
					break;
				}
			}

			for (ndDemoEntity* child = childEntity->GetChild(); child; child = child->GetSibling())
			{
				childEntities[stack] = child;
				parentBone[stack] = parentBody;
				stack++;
			}
		}
	}

	dQuadrupedRobot(const ndLoadSaveBase::ndLoadDescriptor& desc)
		:ndModel(ndLoadSaveBase::ndLoadDescriptor(desc))
		,m_rootBody(nullptr)
		,m_effectors()
		,m_invDynamicsSolver()
		,m_walkParam(0.0f)
	{
		const nd::TiXmlNode* const modelRootNode = desc.m_rootNode;

		const nd::TiXmlNode* const bodies = modelRootNode->FirstChild("bodies");
		for (const nd::TiXmlNode* node = bodies->FirstChild(); node; node = node->NextSibling())
		{
			ndInt32 hashId;
			const nd::TiXmlElement* const element = (nd::TiXmlElement*) node;
			element->Attribute("int32", &hashId);
			ndBodyLoaderCache::ndNode* const bodyNode = desc.m_bodyMap->Find(hashId);

			ndBody* const body = (ndBody*)bodyNode->GetInfo();
			m_bodyArray.PushBack(body->GetAsBodyDynamic());
		}

		const nd::TiXmlNode* const joints = modelRootNode->FirstChild("joints");
		for (const nd::TiXmlNode* node = joints->FirstChild(); node; node = node->NextSibling())
		{
			ndInt32 hashId;
			const nd::TiXmlElement* const element = (nd::TiXmlElement*) node;
			element->Attribute("int32", &hashId);
			ndJointLoaderCache::ndNode* const jointNode = desc.m_jointMap->Find(hashId);

			ndJointBilateralConstraint* const joint = (ndJointBilateralConstraint*)jointNode->GetInfo();
			m_jointArray.PushBack((ndJointHinge*)joint);
		}

		// load root body
		ndBodyLoaderCache::ndNode* const rootBodyNode = desc.m_bodyMap->Find(xmlGetInt(modelRootNode, "rootBodyHash"));
		ndBody* const rootbody = (ndBody*)rootBodyNode->GetInfo();
		m_rootBody = rootbody->GetAsBodyDynamic();
		
		// load effector joint
		const nd::TiXmlNode* const endEffectorNode = modelRootNode->FirstChild("endEffector");
		if (xmlGetInt(endEffectorNode, "hasEffector"))
		{
			dAssert(0);
			//ndBodyLoaderCache::ndNode* const effectorBodyNode0 = desc.m_bodyMap->Find(xmlGetInt(endEffectorNode, "body0Hash"));
			//ndBodyLoaderCache::ndNode* const effectorBodyNode1 = desc.m_bodyMap->Find(xmlGetInt(endEffectorNode, "body1Hash"));
			//
			//ndBody* const body0 = (ndBody*)effectorBodyNode0->GetInfo();
			//ndBody* const body1 = (ndBody*)effectorBodyNode1->GetInfo();
			//dAssert(body1 == m_rootBody);
			//
			//const ndMatrix pivotMatrix(body0->GetMatrix());
			//m_effector = new ndIk6DofEffector(pivotMatrix, body0->GetAsBodyDynamic(), body1->GetAsBodyDynamic());
			//m_effector->EnableRotationAxis(ndIk6DofEffector::m_swivelPlane);
			//m_effector->SetMode(true, true);
			//
			//ndFloat32 regularizer;
			//ndFloat32 springConst;
			//ndFloat32 damperConst;
			//
			//m_effector->GetLinearSpringDamper(regularizer, springConst, damperConst);
			//m_effector->SetLinearSpringDamper(regularizer * 0.5f, springConst * 10.0f, damperConst * 10.0f);
			//
			//m_effector->GetAngularSpringDamper(regularizer, springConst, damperConst);
			//m_effector->SetAngularSpringDamper(regularizer * 0.5f, springConst * 10.0f, damperConst * 10.0f);
		}
	}

	~dQuadrupedRobot()
	{
		for (ndInt32 i = 0; i < m_effectors.GetCount(); i++)
		{
			if (!m_effectors[i]->IsInWorld())
			{
				delete m_effectors[i];
			}
		}
	}

	void Save(const ndLoadSaveBase::ndSaveDescriptor& desc) const
	{
		nd::TiXmlElement* const modelRootNode = new nd::TiXmlElement(ClassName());
		desc.m_rootNode->LinkEndChild(modelRootNode);
		modelRootNode->SetAttribute("hashId", desc.m_nodeNodeHash);
		ndModel::Save(ndLoadSaveBase::ndSaveDescriptor(desc, modelRootNode));

		// save all bodies.
		nd::TiXmlElement* const bodiesNode = new nd::TiXmlElement("bodies");
		modelRootNode->LinkEndChild(bodiesNode);
		for (ndInt32 i = 0; i < m_bodyArray.GetCount(); i++)
		{
			nd::TiXmlElement* const paramNode = new nd::TiXmlElement("body");
			bodiesNode->LinkEndChild(paramNode);

			ndTree<ndInt32, const ndBodyKinematic*>::ndNode* const bodyPartNode = desc.m_bodyMap->Insert(desc.m_bodyMap->GetCount(), m_bodyArray[i]);
			paramNode->SetAttribute("int32", bodyPartNode->GetInfo());
		}

		// save all joints
		nd::TiXmlElement* const jointsNode = new nd::TiXmlElement("joints");
		modelRootNode->LinkEndChild(jointsNode);
		for (ndInt32 i = 0; i < m_jointArray.GetCount(); i++)
		{
			nd::TiXmlElement* const paramNode = new nd::TiXmlElement("joint");
			jointsNode->LinkEndChild(paramNode);

			ndTree<ndInt32, const ndJointBilateralConstraint*>::ndNode* const jointPartNode = desc.m_jointMap->Insert(desc.m_jointMap->GetCount(), m_jointArray[i]);
			paramNode->SetAttribute("int32", jointPartNode->GetInfo());
		}

		// indicate which body is the root
		xmlSaveParam(modelRootNode, "rootBodyHash", desc.m_bodyMap->Find(m_rootBody)->GetInfo());

		// save end effector info
		nd::TiXmlElement* const endEffectorNode = new nd::TiXmlElement("endEffector");
		modelRootNode->LinkEndChild(endEffectorNode);

		dAssert(0);
		//xmlSaveParam(endEffectorNode, "hasEffector", m_effector ? 1 : 0);
		//if (m_effector)
		//{
		//	ndTree<ndInt32, const ndBodyKinematic*>::ndNode* const effectBody0 = desc.m_bodyMap->Find(m_effector->GetBody0());
		//	ndTree<ndInt32, const ndBodyKinematic*>::ndNode* const effectBody1 = desc.m_bodyMap->Find(m_effector->GetBody1());
		//	xmlSaveParam(endEffectorNode, "body0Hash", effectBody0->GetInfo());
		//	xmlSaveParam(endEffectorNode, "body1Hash", effectBody1->GetInfo());
		//}
	}

	ndBodyDynamic* CreateBodyPart(ndDemoEntityManager* const scene, ndDemoEntity* const entityPart, ndFloat32 mass, ndBodyDynamic* const parentBone)
	{
		ndShapeInstance* const shape = entityPart->CreateCollisionFromChildren();
		dAssert(shape);
		
		// create the rigid body that will make this body
		ndMatrix matrix(entityPart->CalculateGlobalMatrix());
		
		ndBodyDynamic* const body = new ndBodyDynamic();
		body->SetMatrix(matrix);
		body->SetCollisionShape(*shape);
		body->SetMassMatrix(mass, *shape);
		body->SetNotifyCallback(new ndDemoEntityNotify(scene, entityPart, parentBone));
		
		delete shape;

		// add body to the world
		scene->GetWorld()->AddBody(body);
		return body;
	}

	ndBodyDynamic* GetRoot() const
	{
		return m_rootBody;
	}

	void Debug(ndConstraintDebugCallback& context) const
	{
		ndFixSizeArray<ndVector , 4> supportPolygon;
		for (ndInt32 i = 0; i < m_effectors.GetCount(); i++)
		{
			dQuadrupedLeg* const joint = m_effectors[i];
			joint->DebugJoint(context);
			if (joint->m_footOnGround)
			{
				ndVector point(joint->GetBody0()->GetMatrix().TransformVector(joint->GetLocalMatrix0().m_posit));
				supportPolygon.PushBack(point);
			}
		}

		// Draw support polygon
		ndVector color(1.0f, 1.0f, 0.0f, 0.0f);
		ndInt32 i0 = supportPolygon.GetCount() - 1;
		for (ndInt32 i = 0; i < supportPolygon.GetCount(); i++)
		{
			context.DrawLine(supportPolygon[i], supportPolygon[i0], color);
			i0 = i;
		}

		ndFloat32 toltalMass = 0.0f;
		ndVector com(ndVector::m_zero);
		for (ndInt32 i = 0; i < m_bodyArray.GetCount(); ++i)
		{
			ndBodyDynamic* const body = m_bodyArray[i];
			ndFloat32 mass = body->GetMassMatrix().m_w;
			ndVector comMass(body->GetMatrix().TransformVector(body->GetCentreOfMass()));
			com += comMass.Scale (mass);
			toltalMass += mass;
		}
		com = com.Scale(1.0f / toltalMass);
		com.m_w = 1.0f;
		ndMatrix rootMatrix(m_rootBody->GetMatrix());
		rootMatrix.m_posit = com;;
		context.DrawFrame(rootMatrix);

		// Draw center of mass projection
		if (supportPolygon.GetCount() >= 3)
		{
			ndBigVector p0(rootMatrix.m_posit);
			p0.m_y -= 1.0f;
			ndBigVector p0Out;
			ndBigVector p1Out;

			ndBigVector poly[16];
			for (ndInt32 i = 0; i < supportPolygon.GetCount(); ++i)
			{
				poly[i] = supportPolygon[i];
			}

			dRayToPolygonDistance(rootMatrix.m_posit, p0, poly, supportPolygon.GetCount(), p0Out, p1Out);
		} 
		else if (supportPolygon.GetCount() == 2)
		{
			ndBigVector p0(rootMatrix.m_posit);
			p0.m_y -= 1.0f;
			ndBigVector p0Out;
			ndBigVector p1Out;
			dRayToRayDistance(rootMatrix.m_posit, p0, supportPolygon[0], supportPolygon[1], p0Out, p1Out);

			ndVector t0(p0Out);
			ndVector t1(p1Out);
			context.DrawLine(t0, t1, ndVector (1.0f, 0.5f, 1.0f, 0.0f));
		}
	}

	void PostUpdate(ndWorld* const world, ndFloat32 timestep)
	{
		ndModel::PostUpdate(world, timestep);
	}

	void PostTransformUpdate(ndWorld* const world, ndFloat32 timestep)
	{
		ndModel::PostTransformUpdate(world, timestep);
	}

	void ApplyControls(ndDemoEntityManager* const scene)
	{
		ndVector color(1.0f, 1.0f, 0.0f, 0.0f);
		scene->Print(color, "Control panel");

		bool change = false;
		//ImGui::Text("ik solver passes");

		//ImGui::Text("position x");
		//change = change | ImGui::SliderFloat("##x", &m_x, 0.0f, 5.0f);
		//ImGui::Text("position y");
		//change = change | ImGui::SliderFloat("##y", &m_y, -1.5f, 2.0f);
		//ImGui::Text("azimuth");
		//change = change | ImGui::SliderFloat("##azimuth", &m_azimuth, -180.0f, 180.0f);
		//
		//ImGui::Text("gripper");
		//change = change | ImGui::SliderFloat("##gripperPosit", &m_gripperPosit, -0.2f, 0.03f);
		//
		//ImGui::Text("pitch");
		//change = change | ImGui::SliderFloat("##pitch", &m_pitch, -180.0f, 180.0f);
		//ImGui::Text("yaw");
		//change = change | ImGui::SliderFloat("##yaw", &m_yaw, -180.0f, 180.0f);
		//ImGui::Text("roll");
		//change = change | ImGui::SliderFloat("##roll", &m_roll, -180.0f, 180.0f);

		if (change)
		{
			m_rootBody->SetSleepState(false);
		}
	}

	void Update(ndWorld* const world, ndFloat32 timestep)
	{
		ndModel::Update(world, timestep);

		ndSkeletonContainer* const skeleton = m_rootBody->GetSkeleton();
		dAssert(skeleton);

		m_rootBody->SetSleepState(false);

		m_invDynamicsSolver.SetMaxIterations(4);
		if (!m_invDynamicsSolver.IsSleeping(skeleton))
		{
			ndFloat32 walkSpeed = 1.0f;
			//ndFloat32 walkSpeed = 0.02f;
			m_walkParam = ndFmod(m_walkParam + walkSpeed * timestep, ndFloat32 (1.0f));
//if (m_walkParam >= 0.5)
//m_walkParam = 0.5;

			for (ndInt32 i = 0; i < m_effectors.GetCount(); i ++)
			{ 
				dQuadrupedLeg* const effector = m_effectors[i];
				effector->GeneratePose(m_walkParam);
				m_invDynamicsSolver.AddEffector(skeleton, effector);
			}
			m_invDynamicsSolver.Solve(skeleton, world, timestep);
		}
	}

	ndMatrix m_referenceFrame;
	ndBodyDynamic* m_rootBody;
	ndFixSizeArray<dQuadrupedLeg*, 4> m_effectors;
	ndIkSolver m_invDynamicsSolver;
	ndFixSizeArray<ndBodyDynamic*, 16> m_bodyArray;
	ndFixSizeArray<ndJointBilateralConstraint*, 16> m_jointArray;
	ndFloat32 m_walkParam;
};

D_CLASS_REFLECTION_IMPLEMENT_LOADER(dQuadrupedRobot);


void RobotControlPanel(ndDemoEntityManager* const scene, void* const context)
{
	dQuadrupedRobot* const me = (dQuadrupedRobot*)context;
	me->ApplyControls(scene);
}

void ndQuadrupedRobot(ndDemoEntityManager* const scene)
{
	// build a floor
	BuildFloorBox(scene, dGetIdentityMatrix());

	ndVector origin1(0.0f, 0.0f, 0.0f, 0.0f);
	fbxDemoEntity* const robotEntity = scene->LoadFbxMesh("spot.fbx");

	ndWorld* const world = scene->GetWorld();
	ndMatrix matrix(dYawMatrix(-90.0f * ndDegreeToRad));

	dQuadrupedRobot* const robot0 = new dQuadrupedRobot(scene, robotEntity, matrix);
	scene->SetSelectedModel(robot0);
	world->AddModel(robot0);
	
	//matrix.m_posit.m_x += 2.0f;
	//matrix.m_posit.m_z -= 2.0f;
	//dQuadrupedRobot* const robot1 = new dQuadrupedRobot(scene, robotEntity, matrix);
	//world->AddModel(robot1);

	delete robotEntity;

	//ndVector posit(matrix.m_posit);
	//posit.m_x += 1.5f;
	//posit.m_z += 1.5f;
	//AddBox(scene, posit, 2.0f, 0.3f, 0.4f, 0.7f);
	//AddBox(scene, posit, 1.0f, 0.3f, 0.4f, 0.7f);

	//posit.m_x += 0.6f;
	//posit.m_z += 0.2f;
	//AddBox(scene, posit, 8.0f, 0.3f, 0.4f, 0.7f);
	//AddBox(scene, posit, 4.0f, 0.3f, 0.4f, 0.7f);

	ndBodyDynamic* const root = robot0->GetRoot();
	//world->AddJoint(new ndJointFix6dof(root->GetMatrix(), root, world->GetSentinelBody()));
	//scene->Set2DDisplayRenderFunction(RobotControlPanel, nullptr, robot0);

	matrix.m_posit.m_x -= 4.5f;
	matrix.m_posit.m_y += 1.5f;
	matrix.m_posit.m_z += 0.5f;
	ndQuaternion rotation(ndVector(0.0f, 1.0f, 0.0f, 0.0f), 0.0f * ndDegreeToRad);
	scene->SetCameraMatrix(rotation, matrix.m_posit);
}
