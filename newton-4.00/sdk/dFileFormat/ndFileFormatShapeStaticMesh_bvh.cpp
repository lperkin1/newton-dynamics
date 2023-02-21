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

#include "ndFileFormatStdafx.h"
#include "ndFileFormatShapeStaticMesh_bvh.h"

ndFileFormatShapeStaticMesh_bvh::ndFileFormatShapeStaticMesh_bvh()
	:ndFileFormatShapeStaticMesh(ndShapeStatic_bvh::StaticClassName())
{
}

ndFileFormatShapeStaticMesh_bvh::ndFileFormatShapeStaticMesh_bvh(const char* const className)
	:ndFileFormatShapeStaticMesh(className)
{
}

void ndFileFormatShapeStaticMesh_bvh::SaveShape(nd::TiXmlElement* const parentNode, const ndShape* const shape)
{
	nd::TiXmlElement* const classNode = xmlCreateClassNode(parentNode, "ndShape", ndShapeStatic_bvh::StaticClassName());
	ndFileFormatShapeStaticMesh::SaveShape(classNode, shape);
	//ndAssert(0);

	ndShapeStatic_bvh* const staticMesh = (ndShapeStatic_bvh*)shape;
	//char fileName[1024];
	//sprintf(fileName, "%s_%d.bin", desc.m_assetName, desc.m_assetIndex);
	//xmlSaveParam(childNode, "assetName", "string", fileName);
	//
	//char filePathName[2 * 1024];
	//sprintf(filePathName, "%s/%s", desc.m_assetPath, fileName);
	//desc.m_assetIndex++;
	//Serialize(filePathName);

}
