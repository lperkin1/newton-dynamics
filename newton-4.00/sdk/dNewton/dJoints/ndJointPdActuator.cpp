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

#include "dCoreStdafx.h"
#include "ndNewtonStdafx.h"
#include "ndJointPdActuator.h"

#define D_PD_MAX_ANGLE	dFloat32 (120.0f * dDegreeToRad)
#define D_PD_PENETRATION_RECOVERY_ANGULAR_SPEED dFloat32 (0.1f) 
#define D_PD_PENETRATION_ANGULAR_LIMIT dFloat32 (10.0f * dDegreeToRad) 

#define D_PD_SMALL_DISTANCE_ERROR dFloat32 (1.0e-2f)
#define D_PD_SMALL_DISTANCE_ERROR2 (D_PD_SMALL_DISTANCE_ERROR * D_PD_SMALL_DISTANCE_ERROR)

D_CLASS_REFLECTION_IMPLEMENT_LOADER(ndJointPdActuator)

ndJointPdActuator::ndJointPdActuator(const dMatrix& pinAndPivotFrame, ndBodyKinematic* const child, ndBodyKinematic* const parent)
	:ndJointBilateralConstraint(8, child, parent, pinAndPivotFrame)
	,m_pivotFrame(m_localMatrix1)
	,m_minTwistAngle(-dFloat32(1.0e10f))
	,m_maxTwistAngle(dFloat32(1.0e10f))
	,m_maxConeAngle(dFloat32(1.0e10f))
	,m_twistAngleSpring(dFloat32(1000.0f))
	,m_twistAngleDamper(dFloat32(50.0f))
	,m_twistAngleRegularizer(dFloat32(5.0e-3f))
	,m_coneAngleSpring(dFloat32(1000.0f))
	,m_coneAngleDamper(dFloat32(50.0f))
	,m_coneAngleRegularizer(dFloat32(5.0e-3f))
	,m_linearSpring(dFloat32(0.0f))
	,m_linearDamper(dFloat32(0.0f))
	,m_linearRegularizer(dFloat32(0.0f))
{
}

ndJointPdActuator::ndJointPdActuator(const dLoadSaveBase::dLoadDescriptor& desc)
	:ndJointBilateralConstraint(dLoadSaveBase::dLoadDescriptor(desc))
	,m_pivotFrame(dGetIdentityMatrix())
	,m_minTwistAngle(-dFloat32(1.0e10f))
	,m_maxTwistAngle(dFloat32(1.0e10f))
	,m_maxConeAngle(dFloat32(1.0e10f))
	,m_twistAngleSpring(dFloat32(1000.0f))
	,m_twistAngleDamper(dFloat32(50.0f))
	,m_twistAngleRegularizer(dFloat32(5.0e-3f))
	,m_coneAngleSpring(dFloat32(1000.0f))
	,m_coneAngleDamper(dFloat32(50.0f))
	,m_coneAngleRegularizer(dFloat32(5.0e-3f))
	,m_linearSpring(dFloat32(1000.0f))
	,m_linearDamper(dFloat32(50.0f))
	,m_linearRegularizer(dFloat32(5.0e-3f))
{
	const nd::TiXmlNode* const xmlNode = desc.m_rootNode;

	m_pivotFrame = xmlGetMatrix(xmlNode, "pivotFrame");
	m_minTwistAngle = xmlGetFloat (xmlNode, "minTwistAngle");
	m_maxTwistAngle = xmlGetFloat (xmlNode, "maxTwistAngle");
	m_twistAngleSpring = xmlGetFloat(xmlNode, "twistAngleSpring");
	m_twistAngleDamper = xmlGetFloat(xmlNode, "twistAngleDamper");
	m_twistAngleRegularizer = xmlGetFloat(xmlNode, "twistAngleRegularizer");

	m_maxConeAngle = xmlGetFloat(xmlNode, "maxConeAngle");
	m_coneAngleSpring = xmlGetFloat (xmlNode, "coneAngleSpring");
	m_coneAngleDamper = xmlGetFloat (xmlNode, "coneAngleDamper");
	m_coneAngleRegularizer = xmlGetFloat (xmlNode, "coneAngleRegularizer");

	m_linearSpring = xmlGetFloat(xmlNode, "linearSpring");
	m_linearDamper = xmlGetFloat(xmlNode, "linearDamper");
	m_linearRegularizer = xmlGetFloat(xmlNode, "linearRegularizer");
}

ndJointPdActuator::~ndJointPdActuator()
{
}

void ndJointPdActuator::Save(const dLoadSaveBase::dSaveDescriptor& desc) const
{
	nd::TiXmlElement* const childNode = new nd::TiXmlElement(ClassName());
	desc.m_rootNode->LinkEndChild(childNode);
	childNode->SetAttribute("hashId", desc.m_nodeNodeHash);
	ndJointBilateralConstraint::Save(dLoadSaveBase::dSaveDescriptor(desc, childNode));

	xmlSaveParam(childNode, "pivotFrame", m_pivotFrame);
	xmlSaveParam(childNode, "minTwistAngle", m_minTwistAngle);
	xmlSaveParam(childNode, "maxTwistAngle", m_maxTwistAngle);
	xmlSaveParam(childNode, "twistAngleSpring", m_twistAngleSpring);
	xmlSaveParam(childNode, "twistAngleDamper", m_twistAngleDamper);
	xmlSaveParam(childNode, "twistAngleRegularizer", m_twistAngleRegularizer);

	xmlSaveParam(childNode, "maxConeAngle", m_maxConeAngle);
	xmlSaveParam(childNode, "coneAngleSpring", m_coneAngleSpring);
	xmlSaveParam(childNode, "coneAngleDamper", m_coneAngleDamper);
	xmlSaveParam(childNode, "coneAngleRegularizer", m_coneAngleRegularizer);

	xmlSaveParam(childNode, "linearSpring", m_linearSpring);
	xmlSaveParam(childNode, "linearDamper", m_linearDamper);
	xmlSaveParam(childNode, "linearRegularizer", m_linearRegularizer);
}

void ndJointPdActuator::GetConeAngleSpringDamperRegularizer(dFloat32& spring, dFloat32& damper, dFloat32& regularizer) const
{
	spring = m_coneAngleSpring;
	damper = m_coneAngleDamper;
	regularizer = m_coneAngleRegularizer;
}

void ndJointPdActuator::SetConeAngleSpringDamperRegularizer(dFloat32 spring, dFloat32 damper, dFloat32 regularizer)
{
	m_coneAngleSpring = dMax(spring, dFloat32(0.0f));
	m_coneAngleDamper = dMax(damper, dFloat32(0.0f));
	m_coneAngleRegularizer = dMax(regularizer, dFloat32(0.0f));
}

void ndJointPdActuator::GetTwistAngleSpringDamperRegularizer(dFloat32& spring, dFloat32& damper, dFloat32& regularizer) const
{
	spring = m_twistAngleSpring;
	damper = m_twistAngleDamper;
	regularizer = m_twistAngleRegularizer;
}

void ndJointPdActuator::SetTwistAngleSpringDamperRegularizer(dFloat32 spring, dFloat32 damper, dFloat32 regularizer)
{
	m_twistAngleSpring = dMax(spring, dFloat32(0.0f));
	m_twistAngleDamper = dMax(damper, dFloat32(0.0f));
	m_twistAngleRegularizer = dMax(regularizer, dFloat32(0.0f));
}

void ndJointPdActuator::SetTwistLimits(dFloat32 minAngle, dFloat32 maxAngle)
{
	m_minTwistAngle = -dAbs(minAngle);
	m_maxTwistAngle = dAbs(maxAngle);
}

void ndJointPdActuator::GetTwistLimits(dFloat32& minAngle, dFloat32& maxAngle) const
{
	minAngle = m_minTwistAngle;
	maxAngle = m_maxTwistAngle;
}

dFloat32 ndJointPdActuator::GetMaxConeAngle() const
{
	return m_maxConeAngle;
}

void ndJointPdActuator::SetConeLimit(dFloat32 maxConeAngle)
{
	m_maxConeAngle = dMin (dAbs(maxConeAngle), D_PD_MAX_ANGLE * dFloat32 (0.999f));
}

dVector ndJointPdActuator::GetTargetPosition() const
{
	return m_localMatrix1.m_posit;
}

void ndJointPdActuator::SetTargetPosition(const dVector& posit)
{
	dAssert(posit.m_w == dFloat32(1.0f));
	m_localMatrix1.m_posit = posit;
}

void ndJointPdActuator::GetLinearSpringDamperRegularizer(dFloat32& spring, dFloat32& damper, dFloat32& regularizer) const
{
	spring = m_linearSpring;
	damper = m_linearDamper;
	regularizer = m_linearRegularizer;
}

void ndJointPdActuator::SetLinearSpringDamperRegularizer(dFloat32 spring, dFloat32 damper, dFloat32 regularizer)
{
	m_linearSpring = dMax(spring, dFloat32(0.0f));
	m_linearDamper = dMax(damper, dFloat32(0.0f));
	m_linearRegularizer = dMax(regularizer, dFloat32(0.0f));
}

dMatrix ndJointPdActuator::GetTargetMatrix() const
{
	return m_localMatrix1;
}

void ndJointPdActuator::SetTargetMatrix(const dMatrix& matrix)
{
	m_localMatrix1 = matrix;
}

void ndJointPdActuator::DebugJoint(ndConstraintDebugCallback& debugCallback) const
{
	dMatrix matrix0;
	dMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);
	matrix1 = m_pivotFrame * m_body1->GetMatrix();

	debugCallback.DrawFrame(matrix0);
	debugCallback.DrawFrame(matrix1);

	const dInt32 subdiv = 8;
	const dVector& coneDir0 = matrix0.m_front;
	const dVector& coneDir1 = matrix1.m_front;
	dFloat32 cosAngleCos = coneDir0.DotProduct(coneDir1).GetScalar();
	dMatrix coneRotation(dGetIdentityMatrix());
	if (cosAngleCos < dFloat32(0.9999f))
	{
		dVector lateralDir(coneDir1.CrossProduct(coneDir0));
		dFloat32 mag2 = lateralDir.DotProduct(lateralDir).GetScalar();
		if (mag2 > dFloat32(1.0e-4f))
		{
			lateralDir = lateralDir.Scale(dFloat32(1.0f) / dSqrt(mag2));
			coneRotation = dMatrix(dQuaternion(lateralDir, dAcos(dClamp(cosAngleCos, dFloat32(-1.0f), dFloat32(1.0f)))), matrix1.m_posit);
		}
		else
		{
			lateralDir = matrix0.m_up.Scale(-dFloat32(1.0f));
			coneRotation = dMatrix(dQuaternion(matrix0.m_up, dFloat32(180.0f) * dDegreeToRad), matrix1.m_posit);
		}
	}
	else if (cosAngleCos < -dFloat32(0.9999f))
	{
		coneRotation[0][0] = dFloat32(-1.0f);
		coneRotation[1][1] = dFloat32(-1.0f);
	}

	const dFloat32 radius = debugCallback.m_debugScale;
	dVector arch[subdiv + 1];

	// show twist angle limits
	dFloat32 deltaTwist = m_maxTwistAngle - m_minTwistAngle;
	if ((deltaTwist > dFloat32(1.0e-3f)) && (deltaTwist < dFloat32(2.0f) * dPi))
	{
		dMatrix pitchMatrix(matrix1 * coneRotation);
		pitchMatrix.m_posit = matrix1.m_posit;

		dVector point(dFloat32(0.0f), dFloat32(radius), dFloat32(0.0f), dFloat32(0.0f));

		dFloat32 angleStep = dMin(m_maxTwistAngle - m_minTwistAngle, dFloat32(2.0f * dPi)) / subdiv;
		dFloat32 angle0 = m_minTwistAngle;

		dVector color(dFloat32(0.4f), dFloat32(0.0f), dFloat32(0.0f), dFloat32(0.0f));
		for (dInt32 i = 0; i <= subdiv; i++)
		{
			arch[i] = pitchMatrix.TransformVector(dPitchMatrix(angle0).RotateVector(point));
			debugCallback.DrawLine(pitchMatrix.m_posit, arch[i], color);
			angle0 += angleStep;
		}

		for (dInt32 i = 0; i < subdiv; i++)
		{
			debugCallback.DrawLine(arch[i], arch[i + 1], color);
		}
	}

	// show cone angle limits
	if ((m_maxConeAngle > dFloat32(0.0f)) && (m_maxConeAngle < D_PD_MAX_ANGLE))
	{
		dVector color(dFloat32(0.3f), dFloat32(0.8f), dFloat32(0.0f), dFloat32(0.0f));
		dVector point(radius * dCos(m_maxConeAngle), radius * dSin(m_maxConeAngle), dFloat32 (0.0f), dFloat32(0.0f));
		dFloat32 angleStep = dPi * dFloat32(2.0f) / subdiv;

		dFloat32 angle0 = dFloat32(0.0f);
		for (dInt32 i = 0; i <= subdiv; i++)
		{
			dVector conePoint(dPitchMatrix(angle0).RotateVector(point));
			dVector p(matrix1.TransformVector(conePoint));
			arch[i] = p;
			debugCallback.DrawLine(matrix1.m_posit, p, color);
			angle0 += angleStep;
		}

		for (dInt32 i = 0; i < subdiv; i++)
		{
			debugCallback.DrawLine(arch[i], arch[i + 1], color);
		}
	}
}

void ndJointPdActuator::SubmitAngularAxisCartesianApproximation(const dMatrix& matrix0, const dMatrix& matrix1, ndConstraintDescritor& desc)
{
	dFloat32 pitchAngle = -CalculateAngle(matrix0[1], matrix1[1], matrix1[0]);
	dFloat32 coneAngle = dAcos(dClamp(matrix1.m_front.DotProduct(matrix0.m_front).GetScalar(), dFloat32(-1.0f), dFloat32(1.0f)));
	if (coneAngle >= m_maxConeAngle)
	{
		dAssert(m_maxConeAngle == dFloat32(0.0f));
		//this is a hinge joint
		AddAngularRowJacobian(desc, matrix0.m_front, -pitchAngle);
		SetMassSpringDamperAcceleration(desc, m_coneAngleRegularizer, m_coneAngleSpring, m_coneAngleDamper);

		// apply cone limits
		// two rows to restrict rotation around the parent coordinate system
		dFloat32 angle0 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up);
		AddAngularRowJacobian(desc, matrix1.m_up, angle0);

		dFloat32 angle1 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right);
		AddAngularRowJacobian(desc, matrix1.m_right, angle1);
	}
	else
	{
		SubmitPdRotation(matrix0, matrix1, desc);
	}

	SubmitTwistLimits(matrix0.m_front, pitchAngle, desc);
}

void ndJointPdActuator::SubmitTwistLimits(const dVector& pin, dFloat32 angle, ndConstraintDescritor& desc)
{
	if((m_maxTwistAngle - m_minTwistAngle) < (2.0f * dDegreeToRad))
	{ 
		AddAngularRowJacobian(desc, pin, -angle);
		// force this limit to be bound
		SetLowerFriction(desc, D_LCP_MAX_VALUE * dFloat32 (0.1f));
	}
	else
	{
		if (angle < m_minTwistAngle)
		{
			AddAngularRowJacobian(desc, pin, dFloat32(0.0f));
			const dFloat32 stopAccel = GetMotorZeroAcceleration(desc);
			const dFloat32 penetration = angle - m_minTwistAngle;
			const dFloat32 recoveringAceel = -desc.m_invTimestep * D_PD_PENETRATION_RECOVERY_ANGULAR_SPEED * dMin(dAbs(penetration / D_PD_PENETRATION_ANGULAR_LIMIT), dFloat32(1.0f));
			SetMotorAcceleration(desc, stopAccel - recoveringAceel);
			SetLowerFriction(desc, dFloat32(0.0f));
		}
		else if (angle >= m_maxTwistAngle)
		{
			AddAngularRowJacobian(desc, pin, dFloat32(0.0f));
			const dFloat32 stopAccel = GetMotorZeroAcceleration(desc);
			const dFloat32 penetration = angle - m_maxTwistAngle;
			const dFloat32 recoveringAceel = desc.m_invTimestep * D_PD_PENETRATION_RECOVERY_ANGULAR_SPEED * dMin(dAbs(penetration / D_PD_PENETRATION_ANGULAR_LIMIT), dFloat32(1.0f));
			SetMotorAcceleration(desc, stopAccel - recoveringAceel);
			SetHighFriction(desc, dFloat32(0.0f));
		}
	}
}

void ndJointPdActuator::SubmitAngularAxis(const dMatrix& matrix0, const dMatrix& matrix1, ndConstraintDescritor& desc)
{
	SubmitPdRotation(matrix0, matrix1, desc);

	dVector lateralDir(matrix1[0].CrossProduct(matrix0[0]));
	dAssert(lateralDir.DotProduct(lateralDir).GetScalar() > 1.0e-6f);
	lateralDir = lateralDir.Normalize();
	dFloat32 coneAngle = dAcos(dClamp(matrix1.m_front.DotProduct(matrix0.m_front).GetScalar(), dFloat32(-1.0f), dFloat32(1.0f)));
	dMatrix coneRotation(dQuaternion(lateralDir, coneAngle), matrix1.m_posit);

	if (coneAngle > m_maxConeAngle)
	{
		AddAngularRowJacobian(desc, lateralDir, 0.0f);
		const dFloat32 stopAccel = GetMotorZeroAcceleration(desc);
		const dFloat32 penetration = coneAngle - m_maxConeAngle;
		const dFloat32 recoveringAceel = desc.m_invTimestep * D_PD_PENETRATION_RECOVERY_ANGULAR_SPEED * dMin(dAbs(penetration / D_PD_PENETRATION_ANGULAR_LIMIT), dFloat32(1.0f));
		SetMotorAcceleration(desc, stopAccel - recoveringAceel);
		SetHighFriction(desc, dFloat32(0.0f));
	}

	dMatrix pitchMatrix(matrix1 * coneRotation * matrix0.Inverse());
	dFloat32 pitchAngle = -dAtan2(pitchMatrix[1][2], pitchMatrix[1][1]);
	SubmitTwistLimits(matrix0.m_front, pitchAngle, desc);
}

void ndJointPdActuator::SubmitPdRotation(const dMatrix& matrix0, const dMatrix& matrix1, ndConstraintDescritor& desc)
{
	dQuaternion q0(matrix0);
	dQuaternion q1(matrix1);
	if (q1.DotProduct(q0).GetScalar() < dFloat32(0.0f))
	{
		q1 = q1.Scale (dFloat32 (-1.0f));
	}

	dQuaternion dq(q1.Inverse() * q0);
	dVector pin(dq.m_x, dq.m_y, dq.m_z, dFloat32(0.0f));

	dFloat32 dirMag2 = pin.DotProduct(pin).GetScalar();
	if (dirMag2 > dFloat32(dFloat32(1.0e-7f)))
	{
		dFloat32 dirMag = dSqrt(dirMag2);
		pin = pin.Scale(dFloat32(1.0f) / dirMag);
		dFloat32 angle = dFloat32(2.0f) * dAtan2(dirMag, dq.m_w);

		dMatrix basis(pin);
		AddAngularRowJacobian(desc, basis[0], -angle);
		SetMassSpringDamperAcceleration(desc, m_coneAngleRegularizer, m_coneAngleSpring, m_coneAngleDamper);

		AddAngularRowJacobian(desc, basis[1], dFloat32 (0.0f));
		SetMassSpringDamperAcceleration(desc, m_coneAngleRegularizer, m_coneAngleSpring, m_coneAngleDamper);

		AddAngularRowJacobian(desc, basis[2], dFloat32(0.0f));
		SetMassSpringDamperAcceleration(desc, m_coneAngleRegularizer, m_coneAngleSpring, m_coneAngleDamper);
	}
	else
	{
		dFloat32 pitchAngle = CalculateAngle(matrix0[1], matrix1[1], matrix1[0]);
		AddAngularRowJacobian(desc, matrix1[0], pitchAngle);
		SetMassSpringDamperAcceleration(desc, m_coneAngleRegularizer, m_coneAngleSpring, m_coneAngleDamper);
		
		dFloat32 yawAngle = CalculateAngle(matrix0[0], matrix1[0], matrix1[1]);
		AddAngularRowJacobian(desc, matrix1[1], yawAngle);
		SetMassSpringDamperAcceleration(desc, m_coneAngleRegularizer, m_coneAngleSpring, m_coneAngleDamper);
		
		dFloat32 rollAngle = CalculateAngle(matrix0[0], matrix1[0], matrix1[2]);
		AddAngularRowJacobian(desc, matrix1[2], rollAngle);
		SetMassSpringDamperAcceleration(desc, m_coneAngleRegularizer, m_coneAngleSpring, m_coneAngleDamper);
	}
}

void ndJointPdActuator::SubmitLinearLimits(const dMatrix& matrix0, const dMatrix& matrix1, ndConstraintDescritor& desc)
{
	if (m_linearRegularizer == dFloat32(0.0f))
	{
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[0]);
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[1]);
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[2]);
	}
	else
	{
		const dVector step(matrix0.m_posit - matrix1.m_posit);
		if (step.DotProduct(step).GetScalar() <= D_PD_SMALL_DISTANCE_ERROR2)
		{
			// Cartesian motion
			AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[0]);
			SetMassSpringDamperAcceleration(desc, m_linearRegularizer, m_linearSpring, m_linearDamper);

			AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[1]);
			SetMassSpringDamperAcceleration(desc, m_linearRegularizer, m_linearSpring, m_linearDamper);

			AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[2]);
			SetMassSpringDamperAcceleration(desc, m_linearRegularizer, m_linearSpring, m_linearDamper);
		}
		else
		{
			const dMatrix basis(step.Normalize());

			// move alone the diagonal;
			AddLinearRowJacobian(desc, matrix1.m_posit, matrix1.m_posit, basis[2]);
			AddLinearRowJacobian(desc, matrix1.m_posit, matrix1.m_posit, basis[1]);
			AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, basis[0]);
			SetMassSpringDamperAcceleration(desc, m_linearRegularizer, m_linearSpring, m_linearDamper);
		}
	}
}

void ndJointPdActuator::JacobianDerivative(ndConstraintDescritor& desc)
{
	dMatrix matrix0;
	dMatrix matrix1;

	CalculateGlobalMatrix(matrix0, matrix1);
	SubmitLinearLimits(matrix0, matrix1, desc);

	if (m_twistAngleRegularizer == dFloat32(0.0f))
	{
	}
	else
	{
		dFloat32 cosAngleCos = matrix1.m_front.DotProduct(matrix0.m_front).GetScalar();
		if (cosAngleCos >= dFloat32(0.998f))
		{
			// special case where the front axis are almost aligned
			// solve by using Cartesian approximation
			SubmitAngularAxisCartesianApproximation(matrix0, matrix1, desc);
		}
		else
		{
			SubmitAngularAxis(matrix0, matrix1, desc);
		}
	}
}