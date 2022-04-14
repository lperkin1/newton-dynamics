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

#include "ndCudaContext.h"

ndCudaDevice::ndCudaDevice()
{
	cudaError_t cudaStatus;
	cudaStatus = cudaGetDeviceProperties(&m_prop, 0);
	dAssert(cudaStatus == cudaSuccess);

	cudaStatus = cudaSetDevice(0);
	dAssert(cudaStatus == cudaSuccess);
	if (cudaStatus != cudaSuccess)
	{
		dAssert(0);
	}
	m_valid = (cudaStatus == cudaSuccess);
}

ndCudaDevice::~ndCudaDevice()
{
	cudaError_t cudaStatus;
	cudaStatus = cudaDeviceReset();
	dAssert(cudaStatus == cudaSuccess);

	if (cudaStatus != cudaSuccess)
	{
		dAssert(0);
	}
}

ndCudaContext::ndCudaContext()
	:ndClassAlloc()
	,ndCudaDevice()
	,m_sceneInfoGpu(nullptr)
	,m_sceneInfoCpu0(nullptr)
	,m_sceneInfoCpu1(nullptr)
	,m_scan()
	,m_histogram()
	,m_bodyBufferCpu(D_GRANULARITY)
	,m_gridHash()
	,m_gridHashTmp()
	,m_bodyBufferGpu()
	,m_boundingBoxGpu()
	,m_transformBufferCpu0()
	,m_transformBufferCpu1()
	,m_transformBufferGpu()
	,m_stream0(0)
{
	cudaError_t cudaStatus;
	// create two streams for double buffer updates
	cudaStatus = cudaStreamCreate(&m_stream0);
	dAssert(cudaStatus == cudaSuccess);

	cudaStatus = cudaMalloc((void**)&m_sceneInfoGpu, sizeof(cuSceneInfo));
	dAssert(cudaStatus == cudaSuccess);

	cudaStatus = cudaMallocHost((void**)&m_sceneInfoCpu0, sizeof(cuSceneInfo));
	dAssert(cudaStatus == cudaSuccess);

	cudaStatus = cudaMallocHost((void**)&m_sceneInfoCpu1, sizeof(cuSceneInfo));
	dAssert(cudaStatus == cudaSuccess);

	if (cudaStatus != cudaSuccess)
	{
		dAssert(0);
	}

	*m_sceneInfoCpu0 = cuSceneInfo();
	*m_sceneInfoCpu1 = cuSceneInfo();

	m_histogram.SetCount(D_GRANULARITY * 4);
	m_gridHash.SetCount(D_GRANULARITY * 4);
	m_gridHashTmp.SetCount(D_GRANULARITY * 4);
}

ndCudaContext::~ndCudaContext()
{
	cudaError_t cudaStatus;

	cudaStatus = cudaFreeHost(m_sceneInfoCpu1);
	dAssert(cudaStatus == cudaSuccess);

	cudaStatus = cudaFreeHost(m_sceneInfoCpu0);
	dAssert(cudaStatus == cudaSuccess);

	cudaStatus = cudaFree(m_sceneInfoGpu);
	dAssert(cudaStatus == cudaSuccess);

	cudaStatus = cudaStreamDestroy(m_stream0);
	dAssert(cudaStatus == cudaSuccess);

	//cudaStatus = cudaStreamDestroy(m_stream1);
	//dAssert(cudaStatus == cudaSuccess);

	if (cudaStatus != cudaSuccess)
	{
		dAssert(0);
	}
}

ndCudaContext* ndCudaContext::CreateContext()
{
	cudaError_t cudaStatus = cudaSetDevice(0);
	ndCudaContext* const context = (cudaStatus == cudaSuccess) ? new ndCudaContext() : nullptr;
	return context;
}

void ndCudaContext::SwapBuffers()
{
	dSwap(m_sceneInfoCpu0, m_sceneInfoCpu1);
	m_transformBufferCpu0.Swap(m_transformBufferCpu1);
}