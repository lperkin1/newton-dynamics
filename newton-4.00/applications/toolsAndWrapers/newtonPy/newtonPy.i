/* Copyright (c) <2003-2021> <Julio Jerez, Newton Game Dynamics>
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

// File : example.i 
%module newtonPy
%{
	#include "testWrapper.h"
%}

%include "testWrapper.h"

//%include "pyBaseNodeInfo.h"
//%template(objInfo) pyBaseNodeInfo<dSceneNodeInfo>;
//%template(meshInfo) pyBaseNodeInfo<dMeshNodeInfo>;
//%template(texInfo) pyBaseNodeInfo<dTextureNodeInfo>;
//%template(matInfo) pyBaseNodeInfo<dMaterialNodeInfo>;
//%template(rigidBidyInfo) pyBaseNodeInfo<dRigidbodyNodeInfo>;


//%include "carrays.i"
//%include "pyTypes.h"
//%include "pyMesh.h"
//%include "pyScene.h"
//%include "pyObject.h"
//%include "pyTexture.h"
//%include "pyMaterial.h"
//%include "pyRigidBody.h"
//%array_class(int, intArray);
//%array_class(double, doubleArray);

