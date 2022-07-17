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

#include "ndCoreStdafx.h"
#include "ndNewtonStdafx.h"
#include "ndJointRoller.h"

#define D_MAX_SLIDER_RECOVERY_SPEED	ndFloat32 (0.5f)
#define D_MAX_SLIDER_PENETRATION	ndFloat32 (0.05f)

#define D_MAX_HINGE_RECOVERY_SPEED	ndFloat32 (0.25f)
#define D_MAX_HINGE_PENETRATION		(ndFloat32 (4.0f) * ndDegreeToRad)

D_CLASS_REFLECTION_IMPLEMENT_LOADER(ndJointRoller)

ndJointRoller::ndJointRoller(const ndMatrix& pinAndPivotFrame, ndBodyKinematic* const child, ndBodyKinematic* const parent)
	:ndJointBilateralConstraint(8, child, parent, pinAndPivotFrame)
	,m_angle(ndFloat32(0.0f))
	,m_omega(ndFloat32(0.0f))
	,m_springKAngle(ndFloat32(0.0f))
	,m_damperCAngle(ndFloat32(0.0f))
	,m_minLimitAngle(ndFloat32(-1.0e10f))
	,m_maxLimitAngle(ndFloat32(1.0e10f))
	,m_offsetAngle(ndFloat32(0.0f))
	,m_springDamperRegularizerAngle(ndFloat32(0.1f))
	,m_posit(ndFloat32(0.0f))
	,m_speed(ndFloat32(0.0f))
	,m_springKPosit(ndFloat32(0.0f))
	,m_damperCPosit(ndFloat32(0.0f))
	,m_minLimitPosit(ndFloat32(-1.0e10f))
	,m_maxLimitPosit(ndFloat32(1.0e10f))
	,m_offsetPosit(ndFloat32(0.0f))
	,m_springDamperRegularizerPosit(ndFloat32(0.1f))
	,m_limitStatePosit(0)
	,m_limitStateAngle(0)
{
}

ndJointRoller::ndJointRoller(const ndMatrix& pinAndPivotInChild, const ndMatrix& pinAndPivotInParent, ndBodyKinematic* const child, ndBodyKinematic* const parent)
	:ndJointBilateralConstraint(8, child, parent, pinAndPivotInChild)
	,m_angle(ndFloat32(0.0f))
	,m_omega(ndFloat32(0.0f))
	,m_springKAngle(ndFloat32(0.0f))
	,m_damperCAngle(ndFloat32(0.0f))
	,m_minLimitAngle(ndFloat32(-1.0e10f))
	,m_maxLimitAngle(ndFloat32(1.0e10f))
	,m_offsetAngle(ndFloat32(0.0f))
	,m_springDamperRegularizerAngle(ndFloat32(0.1f))
	,m_posit(ndFloat32(0.0f))
	,m_speed(ndFloat32(0.0f))
	,m_springKPosit(ndFloat32(0.0f))
	,m_damperCPosit(ndFloat32(0.0f))
	,m_minLimitPosit(ndFloat32(-1.0e10f))
	,m_maxLimitPosit(ndFloat32(1.0e10f))
	,m_offsetPosit(ndFloat32(0.0f))
	,m_springDamperRegularizerPosit(ndFloat32(0.1f))
	,m_limitStatePosit(0)
	,m_limitStateAngle(0)
{
	ndMatrix tmp;
	CalculateLocalMatrix(pinAndPivotInChild, m_localMatrix0, tmp);
	CalculateLocalMatrix(pinAndPivotInParent, tmp, m_localMatrix1);
}

ndJointRoller::ndJointRoller(const ndLoadSaveBase::ndLoadDescriptor& desc)
	:ndJointBilateralConstraint(ndLoadSaveBase::ndLoadDescriptor(desc))
	,m_angle(ndFloat32(0.0f))
	,m_omega(ndFloat32(0.0f))
	,m_springKAngle(ndFloat32(0.0f))
	,m_damperCAngle(ndFloat32(0.0f))
	,m_minLimitAngle(ndFloat32(-1.0e10f))
	,m_maxLimitAngle(ndFloat32(1.0e10f))
	,m_offsetAngle(ndFloat32(0.0f))
	,m_springDamperRegularizerAngle(ndFloat32(0.1f))
	,m_posit(ndFloat32(0.0f))
	,m_speed(ndFloat32(0.0f))
	,m_springKPosit(ndFloat32(0.0f))
	,m_damperCPosit(ndFloat32(0.0f))
	,m_minLimitPosit(ndFloat32(-1.0e10f))
	,m_maxLimitPosit(ndFloat32(1.0e10f))
	,m_offsetPosit(ndFloat32(0.0f))
	,m_springDamperRegularizerPosit(ndFloat32(0.1f))
	,m_limitStatePosit(0)
	,m_limitStateAngle(0)
{
	const nd::TiXmlNode* const xmlNode = desc.m_rootNode;

	m_angle = xmlGetFloat(xmlNode, "angle");
	m_omega = xmlGetFloat(xmlNode, "omega");
	m_springKAngle = xmlGetFloat(xmlNode, "springKAngle");
	m_damperCAngle = xmlGetFloat(xmlNode, "damperCAngle");
	m_minLimitAngle = xmlGetFloat(xmlNode, "minLimitAngle");
	m_maxLimitAngle = xmlGetFloat(xmlNode, "maxLimitAngle");
	m_offsetAngle = xmlGetFloat(xmlNode, "offsetAngle");
	m_springDamperRegularizerAngle = xmlGetFloat(xmlNode, "springDamperRegularizerAngle");

	m_posit = xmlGetFloat(xmlNode, "posit");
	m_speed = xmlGetFloat(xmlNode, "speed");
	m_springKPosit = xmlGetFloat(xmlNode, "springKPosit");
	m_damperCPosit = xmlGetFloat(xmlNode, "damperCPosit");
	m_minLimitPosit = xmlGetFloat(xmlNode, "minLimitPosit");
	m_maxLimitPosit = xmlGetFloat(xmlNode, "maxLimitPosit");
	m_offsetPosit = xmlGetFloat(xmlNode, "offsetPosit");
	m_springDamperRegularizerPosit = xmlGetFloat(xmlNode, "springDamperRegularizerPosit");

	m_limitStatePosit = ndInt8(xmlGetInt(xmlNode, "limitStatePosit"));
	m_limitStateAngle = ndInt8 (xmlGetInt(xmlNode, "limitStateAngle"));
}

ndJointRoller::~ndJointRoller()
{
}

void ndJointRoller::Save(const ndLoadSaveBase::ndSaveDescriptor& desc) const
{
	nd::TiXmlElement* const childNode = new nd::TiXmlElement(ClassName());
	desc.m_rootNode->LinkEndChild(childNode);
	childNode->SetAttribute("hashId", desc.m_nodeNodeHash);
	ndJointBilateralConstraint::Save(ndLoadSaveBase::ndSaveDescriptor(desc, childNode));

	xmlSaveParam(childNode, "angle", m_angle);
	xmlSaveParam(childNode, "omega", m_omega);
	xmlSaveParam(childNode, "springKAngle", m_springKAngle);
	xmlSaveParam(childNode, "damperCAngle", m_damperCAngle);
	xmlSaveParam(childNode, "minLimitAngle", m_minLimitAngle);
	xmlSaveParam(childNode, "maxLimitAngle", m_maxLimitAngle);
	xmlSaveParam(childNode, "offsetAngle", m_offsetAngle);
	xmlSaveParam(childNode, "springDamperRegularizerAngle", m_springDamperRegularizerAngle);

	xmlSaveParam(childNode, "posit", m_posit);
	xmlSaveParam(childNode, "speed", m_speed);
	xmlSaveParam(childNode, "springKPosit", m_springKPosit);
	xmlSaveParam(childNode, "damperCPosit", m_damperCPosit);
	xmlSaveParam(childNode, "minLimitPosit", m_minLimitPosit);
	xmlSaveParam(childNode, "maxLimitPosit", m_maxLimitPosit);
	xmlSaveParam(childNode, "offsetPosit", m_offsetPosit);
	xmlSaveParam(childNode, "springDamperRegularizerPosit", m_springDamperRegularizerPosit);

	xmlSaveParam(childNode, "limitStatePosit", m_limitStatePosit);
	xmlSaveParam(childNode, "limitStateAngle", m_limitStateAngle);
}

ndFloat32 ndJointRoller::GetAngle() const
{
	return m_angle;
}

ndFloat32 ndJointRoller::GetOmega() const
{
	return m_omega;
}

bool ndJointRoller::GetLimitStateAngle() const
{
	return m_limitStateAngle ? true : false;
}

void ndJointRoller::SetLimitStateAngle(bool state)
{
	m_limitStateAngle = state ? 1 : 0;
	if (m_limitStateAngle)
	{
		SetLimitsAngle(m_minLimitAngle, m_maxLimitAngle);
	}
}

void ndJointRoller::SetLimitsAngle(ndFloat32 minLimit, ndFloat32 maxLimit)
{
	dAssert(minLimit <= 0.0f);
	dAssert(maxLimit >= 0.0f);
	m_minLimitAngle = minLimit;
	m_maxLimitAngle = maxLimit;

	if (m_angle > m_maxLimitAngle)
	{
		const ndFloat32 deltaAngle = ndAnglesAdd(m_angle, -m_maxLimitAngle);
		m_angle = m_maxLimitAngle + deltaAngle;
	} 
	else if (m_angle < m_minLimitAngle)
	{
		const ndFloat32 deltaAngle = ndAnglesAdd(m_angle, -m_minLimitAngle);
		m_angle = m_minLimitAngle + deltaAngle;
	}
}

void ndJointRoller::GetLimitsAngle(ndFloat32& minLimit, ndFloat32& maxLimit) const
{
	minLimit = m_minLimitAngle;
	maxLimit = m_maxLimitAngle;
}

ndFloat32 ndJointRoller::GetOffsetAngle() const
{
	return m_offsetAngle;
}

void ndJointRoller::SetOffsetAngle(ndFloat32 angle)
{
	m_offsetAngle = angle;
}

void ndJointRoller::SetAsSpringDamperAngle(ndFloat32 regularizer, ndFloat32 spring, ndFloat32 damper)
{
	m_springKAngle = ndAbs(spring);
	m_damperCAngle = ndAbs(damper);
	m_springDamperRegularizerAngle = ndClamp(regularizer, ndFloat32(1.0e-2f), ndFloat32(0.99f));
}

void ndJointRoller::GetSpringDamperAngle(ndFloat32& regularizer, ndFloat32& spring, ndFloat32& damper) const
{
	spring = m_springKAngle;
	damper = m_damperCAngle;
	regularizer = m_springDamperRegularizerAngle;
}

ndFloat32 ndJointRoller::GetPosit() const
{
	return m_posit;
}

ndFloat32 ndJointRoller::GetOffsetPosit() const
{
	return m_offsetPosit;
}

void ndJointRoller::SetOffsetPosit(ndFloat32 offset)
{
	m_offsetPosit = offset;
}

bool ndJointRoller::GetLimitStatePosit() const
{
	return m_limitStatePosit ? true : false;
}

void ndJointRoller::SetLimitStatePosit(bool state)
{
	m_limitStatePosit = state ? 1 : 0;
}

void ndJointRoller::SetLimitsPosit(ndFloat32 minLimit, ndFloat32 maxLimit)
{
	dAssert(minLimit <= 0.0f);
	dAssert(maxLimit >= 0.0f);
	m_minLimitPosit = minLimit;
	m_maxLimitPosit = maxLimit;
}

void ndJointRoller::GetLimitsPosit(ndFloat32& minLimit, ndFloat32& maxLimit) const
{
	minLimit = m_minLimitPosit;
	maxLimit = m_maxLimitPosit;
}

void ndJointRoller::SetAsSpringDamperPosit(ndFloat32 regularizer, ndFloat32 spring, ndFloat32 damper)
{
	m_springKPosit = ndAbs(spring);
	m_damperCPosit = ndAbs(damper);
	m_springDamperRegularizerPosit = ndClamp(regularizer, ndFloat32(1.0e-2f), ndFloat32(0.99f));
}

void ndJointRoller::GetSpringDamperPosit(ndFloat32& regularizer, ndFloat32& spring, ndFloat32& damper) const
{
	spring = m_springKPosit;
	damper = m_damperCPosit;
	regularizer = m_springDamperRegularizerPosit;
}

void ndJointRoller::DebugJoint(ndConstraintDebugCallback& debugCallback) const
{
	ndMatrix matrix0;
	ndMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);

	debugCallback.DrawFrame(matrix0);
	debugCallback.DrawFrame(matrix1);

	const ndInt32 subdiv = 8;
	const ndFloat32 radius = debugCallback.m_debugScale;
	ndVector arch[subdiv + 1];

	ndFloat32 deltaTwist = m_maxLimitAngle - m_minLimitAngle;
	if ((deltaTwist > ndFloat32(1.0e-3f)) && (deltaTwist <= ndFloat32(2.0f) * ndPi))
	{
		ndMatrix pitchMatrix(matrix1);
		pitchMatrix.m_posit = matrix1.m_posit;

		ndVector point(ndFloat32(0.0f), ndFloat32(radius), ndFloat32(0.0f), ndFloat32(0.0f));

		ndFloat32 angleStep = ndMin(deltaTwist, ndFloat32(2.0f * ndPi)) / subdiv;
		ndFloat32 angle0 = m_minLimitAngle;

		ndVector color(ndFloat32(0.4f), ndFloat32(0.0f), ndFloat32(0.0f), ndFloat32(0.0f));
		for (ndInt32 i = 0; i <= subdiv; ++i)
		{
			arch[i] = pitchMatrix.TransformVector(dPitchMatrix(angle0).RotateVector(point));
			debugCallback.DrawLine(pitchMatrix.m_posit, arch[i], color);
			angle0 += angleStep;
		}

		for (ndInt32 i = 0; i < subdiv; ++i)
		{
			debugCallback.DrawLine(arch[i], arch[i + 1], color);
		}
	}
}

void ndJointRoller::SubmitSpringDamperAngle(ndConstraintDescritor& desc, const ndMatrix& matrix0, const ndMatrix& )
{
	// add spring damper row
	AddAngularRowJacobian(desc, matrix0.m_front, m_offsetAngle - m_angle);
	SetMassSpringDamperAcceleration(desc, m_springDamperRegularizerAngle, m_springKAngle, m_damperCAngle);
}

void ndJointRoller::SubmitSpringDamperPosit(ndConstraintDescritor& desc, const ndMatrix& matrix0, const ndMatrix& matrix1)
{
	// add spring damper row
	const ndVector p1(matrix1.m_posit + matrix1.m_up.Scale(m_offsetPosit));
	AddLinearRowJacobian(desc, matrix0.m_posit, p1, matrix1.m_up);
	SetMassSpringDamperAcceleration(desc, m_springDamperRegularizerPosit, m_springKPosit, m_damperCPosit);
}

void ndJointRoller::ApplyBaseRows(ndConstraintDescritor& desc, const ndMatrix& matrix0, const ndMatrix& matrix1)
{
	const ndVector veloc0(m_body0->GetVelocityAtPoint(matrix0.m_posit));
	const ndVector veloc1(m_body1->GetVelocityAtPoint(matrix1.m_posit));

	const ndVector& pin = matrix1[0];
	const ndVector& p0 = matrix0.m_posit;
	const ndVector& p1 = matrix1.m_posit;
	const ndVector prel(p0 - p1);
	const ndVector vrel(veloc0 - veloc1);

	m_speed = vrel.DotProduct(matrix1.m_up).GetScalar();
	m_posit = prel.DotProduct(matrix1.m_up).GetScalar();
	const ndVector projectedPoint = p1 + pin.Scale(pin.DotProduct(prel).GetScalar());

	AddLinearRowJacobian(desc, p0, projectedPoint, matrix1[0]);
	AddLinearRowJacobian(desc, p0, projectedPoint, matrix1[2]);

	//const ndFloat32 angle0 = CalculateAngle(matrix0.m_up, matrix1.m_up, matrix1.m_front);
	//AddAngularRowJacobian(desc, matrix1.m_front, angle0);

	const ndFloat32 angle1 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up);
	AddAngularRowJacobian(desc, matrix1.m_up, angle1);

	const ndFloat32 angle2 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right);
	AddAngularRowJacobian(desc, matrix1.m_right, angle2);
	
	// save the current joint Omega
	const ndVector omega0(m_body0->GetOmega());
	const ndVector omega1(m_body1->GetOmega());
	
	// the joint angle can be determined by getting the angle between any two non parallel vectors
	const ndFloat32 deltaAngle = ndAnglesAdd(-CalculateAngle(matrix0.m_up, matrix1.m_up, matrix1.m_front), -m_angle);
	m_angle += deltaAngle;
	m_omega = matrix1.m_front.DotProduct(omega0 - omega1).GetScalar();
}

ndFloat32 ndJointRoller::PenetrationOmega(ndFloat32 penetration) const
{
	ndFloat32 param = ndClamp(penetration, ndFloat32(0.0f), D_MAX_HINGE_PENETRATION) / D_MAX_HINGE_PENETRATION;
	ndFloat32 omega = D_MAX_HINGE_RECOVERY_SPEED * param;
	return omega;
}

ndInt8 ndJointRoller::SubmitLimitsAngle(ndConstraintDescritor& desc, const ndMatrix& matrix0, const ndMatrix& matrix1)
{
	ndInt8 ret = 0;
	if (m_limitStateAngle)
	{
		if ((m_minLimitAngle > (ndFloat32(-1.0f) * ndDegreeToRad)) && (m_maxLimitAngle < (ndFloat32(1.0f) * ndDegreeToRad)))
		{
			AddAngularRowJacobian(desc, &matrix1.m_front[0], -m_angle);
			ret = 1;
		}
		else
		{
			const ndFloat32 angle = m_angle + m_omega * desc.m_timestep;
			if (angle < m_minLimitAngle)
			{
				AddAngularRowJacobian(desc, &matrix0.m_front[0], ndFloat32(0.0f));
				const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
				const ndFloat32 penetration = angle - m_minLimitAngle;
				const ndFloat32 recoveringAceel = -desc.m_invTimestep * PenetrationOmega(-penetration);
				SetMotorAcceleration(desc, stopAccel - recoveringAceel);
				SetLowerFriction(desc, ndFloat32(0.0f));
				ret = ndAbs(stopAccel) > ND_MAX_STOP_ACCEL;
			}
			else if (angle > m_maxLimitAngle)
			{
				AddAngularRowJacobian(desc, &matrix0.m_front[0], ndFloat32(0.0f));
				const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
				const ndFloat32 penetration = angle - m_maxLimitAngle;
				const ndFloat32 recoveringAceel = desc.m_invTimestep * PenetrationOmega(penetration);
				SetMotorAcceleration(desc, stopAccel - recoveringAceel);
				SetHighFriction(desc, ndFloat32(0.0f));
				ret = ndAbs(stopAccel) > ND_MAX_STOP_ACCEL;
			}
		}
	}
	return ret;
}

ndFloat32 ndJointRoller::PenetrationSpeed(ndFloat32 penetration) const
{
	ndFloat32 param = ndClamp(penetration, ndFloat32(0.0f), D_MAX_SLIDER_PENETRATION) / D_MAX_SLIDER_PENETRATION;
	ndFloat32 speed = D_MAX_SLIDER_RECOVERY_SPEED * param;
	return speed;
}

ndInt8 ndJointRoller::SubmitLimitsPosit(ndConstraintDescritor& desc, const ndMatrix& matrix0, const ndMatrix& matrix1)
{
	ndInt8 ret = false;
	if (m_limitStatePosit)
	{
		if ((m_minLimitPosit == ndFloat32(0.0f)) && (m_maxLimitPosit == ndFloat32(0.0f)))
		{
			AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1.m_front);
			ret = 1;
		}
		else
		{
			ndFloat32 x = m_posit + m_speed * desc.m_timestep;
			if (x < m_minLimitPosit)
			{
				ndVector p1(matrix1.m_posit + matrix1.m_front.Scale(m_minLimitPosit));
				AddLinearRowJacobian(desc, matrix0.m_posit, p1, matrix1.m_front);
				const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
				const ndFloat32 penetration = x - m_minLimitPosit;
				const ndFloat32 recoveringAceel = -desc.m_invTimestep * PenetrationSpeed(-penetration);
				SetMotorAcceleration(desc, stopAccel - recoveringAceel);
				SetLowerFriction(desc, ndFloat32(0.0f));
				ret = ndAbs(stopAccel) > ND_MAX_STOP_ACCEL;
			}
			else if (x > m_maxLimitPosit)
			{
				AddLinearRowJacobian(desc, matrix0.m_posit, matrix0.m_posit, matrix1.m_front);
				const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
				const ndFloat32 penetration = x - m_maxLimitPosit;
				const ndFloat32 recoveringAceel = desc.m_invTimestep * PenetrationSpeed(penetration);
				SetMotorAcceleration(desc, stopAccel - recoveringAceel);
				SetHighFriction(desc, ndFloat32(0.0f));
				ret = ndAbs(stopAccel) > ND_MAX_STOP_ACCEL;
			}
		}
	}
	return ret;
}

void ndJointRoller::JacobianDerivative(ndConstraintDescritor& desc)
{
	ndMatrix matrix0;
	ndMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);

	ApplyBaseRows(desc, matrix0, matrix1);
	ndInt8 hitLimitAngle = SubmitLimitsAngle(desc, matrix0, matrix1);
	if (!hitLimitAngle)
	{
		if ((m_springKAngle > ndFloat32(0.0f)) || (m_damperCAngle > ndFloat32(0.0f)))
		{
			// spring damper with limits
			SubmitSpringDamperAngle(desc, matrix0, matrix1);
		}
	}

	ndInt8 hitLimitPosit = SubmitLimitsPosit(desc, matrix0, matrix1);
	if (!hitLimitPosit)
	{
		if ((m_springKPosit > ndFloat32(0.0f)) || (m_damperCPosit > ndFloat32(0.0f)))
		{
			// spring damper with limits
			SubmitSpringDamperPosit(desc, matrix0, matrix1);
		}
	}
}


