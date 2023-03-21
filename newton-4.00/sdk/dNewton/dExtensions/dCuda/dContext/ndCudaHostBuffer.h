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

#ifndef __ND_CUDA_HOST_BUFFER_H__
#define __ND_CUDA_HOST_BUFFER_H__

#include <cuda.h>
#include <cuda_runtime.h>
#include "ndCudaUtils.h"
#include "ndCudaIntrinsics.h"

//#define D_HOST_SORT_BLOCK_SIZE	(8)
//#define D_HOST_SORT_BLOCK_SIZE	(16)
#define D_HOST_SORT_BLOCK_SIZE	(1<<8)
//#define D_HOST_SORT_BLOCK_SIZE	(1<<9)
//#define D_HOST_SORT_BLOCK_SIZE	(1<<10)

#define D_HOST_MAX_RADIX_SIZE	(1<<8)
//#define D_HOST_MAX_RADIX_SIZE	(1<<3)

#if D_HOST_MAX_RADIX_SIZE > D_HOST_SORT_BLOCK_SIZE
	#error counting sort diget larger that block
#endif

template<class T>
class ndCudaHostBuffer
{
	public:
	ndCudaHostBuffer();
	~ndCudaHostBuffer();

	int GetCount() const;
	void SetCount(int count);

	void Clear();
	void Resize(int count);
	int GetCapacity() const;

	T& operator[] (int i);
	const T& operator[] (int i) const;

	void Swap(ndCudaHostBuffer& buffer);

	void ReadData(const T* const src, int elements);
	void WriteData(T* const dst, int elements) const;

	void ReadData(const T* const src, int elements, cudaStream_t stream);
	void WriteData(T* const dst, int elements, cudaStream_t stream) const;

	T* m_array;
	int m_size;
	int m_capacity;
};

template <class T, class ndEvaluateKey, int exponentRadix>
void ndCountingSort(const ndCudaHostBuffer<T>& src, ndCudaHostBuffer<T>& dst, ndCudaHostBuffer<int>& scansBuffer);

template<class T>
ndCudaHostBuffer<T>::ndCudaHostBuffer()
	:m_array(nullptr)
	,m_size(0)
	,m_capacity(0)
{
	SetCount(D_GRANULARITY);
	SetCount(0);
}

template<class T>
ndCudaHostBuffer<T>::~ndCudaHostBuffer()
{
	if (m_array)
	{
		cudaError_t cudaStatus = cudaSuccess;
		cudaStatus = cudaFreeHost(m_array);
		ndAssert(cudaStatus == cudaSuccess);
		if (cudaStatus != cudaSuccess)
		{
			ndAssert(0);
		}
	}
}

template<class T>
const T& ndCudaHostBuffer<T>::operator[] (int i) const
{
	ndAssert(i >= 0);
	ndAssert(i < m_size);
	return m_array[i];
}

template<class T>
T& ndCudaHostBuffer<T>::operator[] (int i)
{
	ndAssert(i >= 0);
	ndAssert(i < m_size);
	return m_array[i];
}

template<class T>
int ndCudaHostBuffer<T>::GetCount() const
{
	return m_size;
}

template<class T>
void ndCudaHostBuffer<T>::SetCount(int count)
{
	while (count > m_capacity)
	{
		Resize(m_capacity * 2);
	}
	m_size = count;
}

template<class T>
int ndCudaHostBuffer<T>::GetCapacity() const
{
	return m_capacity;
}

template<class T>
void ndCudaHostBuffer<T>::Clear()
{
	m_size = 0;
}

template<class T>
void ndCudaHostBuffer<T>::Swap(ndCudaHostBuffer& buffer)
{
	cuSwap(m_size, buffer.m_size);
	cuSwap(m_array, buffer.m_array);
	cuSwap(m_capacity, buffer.m_capacity);
}

template<class T>
void ndCudaHostBuffer<T>::Resize(int newSize)
{
	cudaError_t cudaStatus = cudaSuccess;
	if (newSize > m_capacity || (m_capacity == 0))
	{
		T* newArray;
		newSize = std::max(newSize, D_GRANULARITY);
		cudaStatus = cudaMallocHost((void**)&newArray, newSize * sizeof(T));
		ndAssert(cudaStatus == cudaSuccess);
		if (m_array)
		{
			cudaStatus = cudaMemcpy(newArray, m_array, m_size * sizeof(T), cudaMemcpyDeviceToDevice);
			ndAssert(cudaStatus == cudaSuccess);
			cudaStatus = cudaFreeHost(m_array);
			ndAssert(cudaStatus == cudaSuccess);
		}
		m_array = newArray;
		m_capacity = newSize;
	}
	else if (newSize < m_capacity)
	{
		T* newArray;
		newSize = std::max(newSize, D_GRANULARITY);
		cudaStatus = cudaMallocHost((void**)&newArray, newSize * sizeof(T));
		if (m_array)
		{
			cudaStatus = cudaMemcpy(newArray, m_array, newSize * sizeof(T), cudaMemcpyDeviceToDevice);
			cudaStatus = cudaFreeHost(m_array);
			ndAssert(cudaStatus == cudaSuccess);
		}

		m_capacity = newSize;
		m_array = newArray;
	}
	if (cudaStatus != cudaSuccess)
	{
		ndAssert(0);
	}
}

template<class T>
void ndCudaHostBuffer<T>::ReadData(const T* const src, int elements)
{
	ndAssert(elements <= m_size);
	cudaMemcpy(m_array, src, sizeof (T) * elements, cudaMemcpyHostToDevice);
}

template<class T>
void ndCudaHostBuffer<T>::WriteData(T* const dst, int elements) const
{
	ndAssert(elements <= m_size);
	cudaMemcpy(dst, m_array, sizeof(T) * elements, cudaMemcpyDeviceToHost);
}

template<class T>
void ndCudaHostBuffer<T>::ReadData(const T* const src, int elements, cudaStream_t stream)
{
	ndAssert(elements <= m_size);
	cudaError_t cudaStatus = cudaMemcpyAsync(m_array, src, sizeof(T) * elements, cudaMemcpyHostToDevice, stream);
	ndAssert(cudaStatus == cudaSuccess);
	if (cudaStatus != cudaSuccess)
	{
		ndAssert(0);
	}
}

template<class T>
void ndCudaHostBuffer<T>::WriteData(T* const dst, int elements, cudaStream_t stream) const
{
	ndAssert(elements <= m_size);
	cudaError_t cudaStatus = cudaMemcpyAsync(dst, m_array, sizeof(T) * elements, cudaMemcpyDeviceToHost, stream);
	ndAssert(cudaStatus == cudaSuccess);
	if (cudaStatus != cudaSuccess)
	{
		ndAssert(0);
	}
}

#define D_LOG_BANK_COUNT 5
#define D_BANK_COUNT	(1<<D_LOG_BANK_COUNT)
#define D_BANK_PADD		1
template <int size>
class ndBankFreeArray
{
	public:
	ndBankFreeArray()
	{
	}

	int GetBankAddress(int address) const
	{
		int low = address & (D_BANK_COUNT-1);
		int high = address >> D_LOG_BANK_COUNT;
		int dst = high * (D_BANK_COUNT + D_BANK_PADD) + low;
		return dst;
	}

	int& operator[] (int address)
	{
		int low = address & (D_BANK_COUNT - 1);
		int high = address >> D_LOG_BANK_COUNT;
		int index = GetBankAddress(address);
		return m_array[index];
	}

	//int m_array[(size + D_BANK_COUNT - 1) >> D_LOG_BANK_COUNT][D_BANK_COUNT + D_BANK_PADD];
	int m_array[((size + D_BANK_COUNT - 1) >> D_LOG_BANK_COUNT) * (D_BANK_COUNT + D_BANK_PADD)];
};

template <class T, class ndEvaluateKey, int exponentRadix>
void ndCountingSort(const ndCudaHostBuffer<T>& src, ndCudaHostBuffer<T>& dst, ndCudaHostBuffer<int>& scansBuffer)
{
	auto AddPrefix = [&](int blockIdx, int blockDim, int computeUnits)
	{
		int sum[D_HOST_MAX_RADIX_SIZE];
		int offset[D_HOST_MAX_RADIX_SIZE];
		int localPrefixScan[D_HOST_MAX_RADIX_SIZE / 2 + D_HOST_MAX_RADIX_SIZE + 1];
		
		for (int threadId = 0; threadId < blockDim; ++threadId)
		{
			sum[threadId] = 0;
			localPrefixScan[threadId] = 0;
			offset[threadId] = threadId;
		}
		
		for (int i = 0; i < computeUnits; ++i)
		{
			for (int threadId = 0; threadId < blockDim; ++threadId)
			{
				int count = scansBuffer[offset[threadId]];
				scansBuffer[offset[threadId]] = sum[threadId];
				sum[threadId] += count;
				offset[threadId] += blockDim;
			}
		}
		
		for (int threadId = 0; threadId < blockDim; ++threadId)
		{
			localPrefixScan[blockDim / 2 + threadId + 1] = sum[threadId];
		}
		
		for (int i = 1; i < blockDim; i = i << 1)
		{
			for (int threadId = 0; threadId < blockDim; ++threadId)
			{
				sum[threadId] = localPrefixScan[blockDim / 2 + threadId] + localPrefixScan[blockDim / 2 - i + threadId];
			}
			for (int threadId = 0; threadId < blockDim; ++threadId)
			{
				localPrefixScan[blockDim / 2 + threadId] = sum[threadId];
			}
		}
		
		for (int threadId = 0; threadId < blockDim; ++threadId)
		{
			scansBuffer[offset[threadId]] = localPrefixScan[blockDim / 2 + threadId];
		}
	};

	auto CountItems = [&](int blockIndex, int blocksCount)
	{
		int radixCountBuffer[D_HOST_MAX_RADIX_SIZE];
		
		int size = src.GetCount();
		int radixStride = 1 << exponentRadix;
		int blockStride = D_HOST_SORT_BLOCK_SIZE;
		int bashSize = blocksCount * blockStride * blockIndex;
		
		ndEvaluateKey evaluator;
		for (int threadId = 0; threadId < radixStride; ++threadId)
		{
			radixCountBuffer[threadId] = 0;
		}
		
		for (int i = 0; i < blocksCount; ++i)
		{
			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int index = bashSize + threadId;
				if (index < size)
				{
					int radix = evaluator.GetRadix(src[index]);
					radixCountBuffer[radix] ++;
				}
			}
			bashSize += blockStride;
		}
		
		for (int threadId = 0; threadId < radixStride; ++threadId)
		{
			int index = threadId + radixStride * blockIndex;
			scansBuffer[index] = radixCountBuffer[threadId];
		}
	};

//#define D_USE_CPU_BANK_COUNTING_SORT
//#define D_USE_CPU_LOCAL_BITONIC_SORT
//#define D_USE_CPU_LOCAL_COUNTING_SORT
#ifdef D_USE_CPU_BANK_COUNTING_SORT

	auto ShufleUp = [&](const int* in, int* out, int offset)
	{
		for (int i = 0; i < offset; ++i)
		{
			out[i] = in[i];
		}

		for (int i = D_BANK_COUNT - offset - 1; i >= 0; --i)
		{
			out[i + offset] = in[i];
		}
	};

	auto MergeBuckects = [&](int blockIdx, int blocksCount, int computeUnits)
	{
		//T cachedItems[D_HOST_SORT_BLOCK_SIZE];
		//int radixPrefixCount[D_HOST_MAX_RADIX_SIZE];
		//int radixPrefixStart[D_HOST_MAX_RADIX_SIZE];
		//ndBankFreeArray<D_HOST_SORT_BLOCK_SIZE> sortedRadix;
		//ndBankFreeArray<D_HOST_MAX_RADIX_SIZE> radixPrefixScan;

		int sortedRadix[D_HOST_SORT_BLOCK_SIZE];
		int dstLocalOffset[D_HOST_SORT_BLOCK_SIZE];
		//ndBankFreeArray<D_HOST_MAX_RADIX_SIZE> radixPrefixScan;

		int size = src.GetCount();
		int radixStride = (1 << exponentRadix);
		int blockStride = D_HOST_SORT_BLOCK_SIZE;
		int radixBase = blockIdx * radixStride;
		int bashSize = blocksCount * blockStride * blockIdx;
		int radixPrefixOffset = computeUnits * radixStride;

		ndEvaluateKey evaluator;
		//for (int threadId = 0; threadId < radixStride; ++threadId)
		//{
		//	int a = scansBuffer[radixBase + threadId];
		//	int b = scansBuffer[radixPrefixOffset + threadId];
		//	radixPrefixStart[threadId] = a + b;
		//	radixPrefixScan[threadId] = 0;
		//}

		for (int i = 0; i < blocksCount; ++i)
		{
			//for (int threadId = 0; threadId < radixStride; ++threadId)
			//{
			//	radixPrefixCount[threadId] = 0;
			//}

			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int index = bashSize + threadId;
				//int sortKey = radixStride << 16;
				int sortKey = radixStride;
				if (index < size)
				{
					const T item(src[index]);
					//cachedItems[threadId] = src[index];
					int radix = evaluator.GetRadix(item);
					//radixPrefixCount[radix] ++;
					//sortKey = (radix << 16) + threadId;
					sortKey = radix;
				}
				sortedRadix[threadId] = sortKey;
			}


			for (int bank = 0; bank < blockStride; bank += D_BANK_COUNT)
			{
				//for (int bit = 11; bit < radixStride; bit <<= 2)
				for (int bit = 0; bit < 4; ++bit)
				{
					int keyReg[D_BANK_COUNT];
					for (int threadId = 0; threadId < D_BANK_COUNT; ++threadId)
					{
						keyReg[threadId] = sortedRadix[bank + threadId];
					}

					int radixPrefixScanReg[D_BANK_COUNT + 1];
					for (int threadId = 0; threadId < D_BANK_COUNT; ++threadId)
					{
						int test = (keyReg[threadId] >> (bit * 2)) & 0x3;
						dstLocalOffset[threadId] = test;
						radixPrefixScanReg[threadId] = 1 << (test << 3);
					}

					for (int n = 1; n < D_BANK_COUNT; n *= 2)
					{
						int radixPrefixScanReg1[D_BANK_COUNT];
						ShufleUp(radixPrefixScanReg, radixPrefixScanReg1, n);
						for (int threadId = 0; threadId < D_BANK_COUNT; ++threadId)
						{
							if ((threadId & (D_BANK_COUNT - 1)) >= n)
							{
								radixPrefixScanReg[threadId] += radixPrefixScanReg1[threadId];
							}
						}
					}

					int base = radixPrefixScanReg[D_BANK_COUNT - 1];
					base = base + (base << 8);
					base = (base + (base << 16)) << 8;
					for (int threadId = D_BANK_COUNT - 1; threadId >= 0; --threadId)
					{
						radixPrefixScanReg[threadId + 1] = radixPrefixScanReg[threadId];
					}
					radixPrefixScanReg[0] = 0;

					for (int threadId = 0; threadId < D_BANK_COUNT; ++threadId)
					{
						int key = radixPrefixScanReg[threadId];
						int shift = dstLocalOffset[threadId] << 3;
						int keyIndex = ((key + base) >> shift) & 0xff;
						dstLocalOffset[threadId] = keyIndex;
					}

					for (int threadId = 0; threadId < D_BANK_COUNT; ++threadId)
					{
						int dstIndex = dstLocalOffset[threadId];
						sortedRadix[bank + dstIndex] = keyReg[threadId];
					}
				}
				cuTrace(("\n"));
			}

			cuTrace(("\n"));

			//for (int threadId = 0; threadId < blockStride; ++threadId)
			//{
			//	int index = bashSize + threadId;
			//	if (index < size)
			//	{
			//		int keyValue = sortedRadix[threadId];
			//		int keyHigh = keyValue >> 16;
			//		int keyLow = keyValue & 0xffff;
			//		int dstOffset1 = radixPrefixStart[keyHigh];
			//		int dstOffset0 = threadId - radixPrefixScan[radixStride / 2 + keyHigh];
			//		dst[dstOffset0 + dstOffset1] = cachedItems[keyLow];
			//	}
			//}
			//
			//for (int threadId = 0; threadId < radixStride; ++threadId)
			//{
			//	radixPrefixStart[threadId] += radixPrefixCount[threadId];
			//}

			bashSize += blockStride;
		}
	};


	//#ifdef D_USE_CPU_LOCAL_BITONIC_SORT
#elif defined (D_USE_CPU_LOCAL_BITONIC_SORT)
	auto MergeBuckects = [&](int blockIdx, int blocksCount, int computeUnits)
	{
		T cachedItems[D_HOST_SORT_BLOCK_SIZE];
		//int sortedRadix[D_HOST_SORT_BLOCK_SIZE];
		int radixPrefixCount[D_HOST_MAX_RADIX_SIZE];
		int radixPrefixStart[D_HOST_MAX_RADIX_SIZE];
		ndBankFreeArray<D_HOST_SORT_BLOCK_SIZE> sortedRadix;
		//int radixPrefixScan[D_HOST_MAX_RADIX_SIZE / 2 + D_HOST_MAX_RADIX_SIZE + 1];
		ndBankFreeArray<D_HOST_MAX_RADIX_SIZE> radixPrefixScan;

		int size = src.GetCount();
		int radixStride = (1 << exponentRadix);
		int blockStride = D_HOST_SORT_BLOCK_SIZE;
		int radixBase = blockIdx * radixStride;
		int bashSize = blocksCount * blockStride * blockIdx;
		int radixPrefixOffset = computeUnits * radixStride;

		ndEvaluateKey evaluator;
		for (int threadId = 0; threadId < radixStride; ++threadId)
		{
			int a = scansBuffer[radixBase + threadId];
			int b = scansBuffer[radixPrefixOffset + threadId];
			radixPrefixStart[threadId] = a + b;
			radixPrefixScan[threadId] = 0;
		}

		for (int i = 0; i < blocksCount; ++i)
		{
			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixCount[threadId] = 0;
			}

			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int index = bashSize + threadId;
				if (index < size)
				{
					cachedItems[threadId] = src[index];
					int radix = evaluator.GetRadix(cachedItems[threadId]);
					radixPrefixCount[radix] ++;
					sortedRadix[threadId] = (radix << 16) + threadId;
				}
				else
				{
					sortedRadix[threadId] = (radixStride << 16);
				}
			}

#if 1
			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				//radixPrefixScan[threadId] = radixPrefixCount[threadId];
				radixPrefixScan[threadId] = 1;
			}

			//radixStride = 64;
			int radixBit = 0;
			int xxxxx = 64;
			for (int k = 1; k < xxxxx; k = k * 2)
			{
				radixBit++;
				//for (int threadId = 0; threadId < (radixStride >> radixBit); threadId++)
				//{
				//	cuTrace(("%d ", threadId));
				//}
				//cuTrace(("\n"));

				//cuTrace(("thread=%d bit=%d base=%d\n", k, radixBit, base));
				cuTrace(("bit=%d\n", radixBit));
				for (int base = 0; base < radixStride; base += 64)
				{
					for (int threadId = 0; threadId < (xxxxx >> radixBit); threadId++)
					{
						int bankIndex = threadId & 0x1f;
						//int id0 = ((threadId + 1) << radixBit) - 1;
						int id1 = ((bankIndex + 1) << radixBit) - 1;
						int id0 = id1 - (1 << (radixBit - 1));
						//cuTrace(("%d ", sortedRadix.GetBankAddress(id0) % D_BANK_COUNT));
						//cuTrace(("%d ", base + id0));
						cuTrace(("%d ", id0));
					}
					cuTrace(("\n"));
				}
				cuTrace(("\n"));

				//for (int threadId = 0; threadId < (radixStride >> radixBit); threadId++)
				//{
				//	int id1 = ((threadId + 1) << radixBit) - 1;
				//	int id0 = id1 - (1 << (radixBit - 1));
				//	//cuTrace(("%d ", sortedRadix.GetBankAddress(id0) % D_BANK_COUNT));
				//	//cuTrace(("%d ", id0));
				//
				//	//int a = radixPrefixScan[id1];
				//	//int b = radixPrefixScan[id0];
				//	radixPrefixScan[id1] += radixPrefixScan[id0];
				//}
				//cuTrace(("\n"));

				//for (int threadId = 0; threadId < radixStride; threadId++)
				//{
				//	cuTrace(("%d ", radixPrefixScan[threadId]));
				//}
				//cuTrace(("\n"));
				//cuTrace(("\n"));
			}

			//radixPrefixScan[radixStride - 1] = 0;
			//for (int k = 1; k < radixStride; k = k * 2)
			//{
			//	//for (int threadId = 0; threadId < k; threadId++)
			//	//{
			//	//	cuTrace(("%d ", threadId));
			//	//}
			//	//cuTrace(("\n"));
			//
			//	for (int threadId = 0; threadId < k; threadId++)
			//	{
			//		int id0 = ((threadId + 1) << radixBit) - 1;
			//		cuTrace(("%d ", id0));
			//	}
			//	cuTrace(("\n"));
			//
			//	for (int threadId = 0; threadId < k; threadId++)
			//	{
			//		int id1 = ((threadId + 1) << radixBit) - 1;
			//		int id0 = id1 - (1 << (radixBit - 1));
			//		cuTrace(("%d ", id1));
			//
			//		int a = radixPrefixScan[id1];
			//		int b = radixPrefixScan[id0] + a;
			//
			//		radixPrefixScan[id0] = a;
			//		radixPrefixScan[id1] = b;
			//	}
			//	cuTrace(("\n"));
			//
			//	//for (int threadId = 0; threadId < radixStride; threadId++)
			//	//{
			//	//	cuTrace(("%d ", radixPrefixScan[threadId]));
			//	//}
			//	//cuTrace(("\n"));
			//	
			//	cuTrace(("\n"));
			//	radixBit--;
			//}

			//for (int threadId = 0; threadId < radixStride; threadId++)
			//{
			//	cuTrace(("%d ", radixPrefixScan[threadId]));
			//}
			cuTrace(("\n"));
#else
			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixScan[radixStride / 2 + threadId + 1] = radixPrefixCount[threadId];
			}
			for (int k = 1; k < radixStride; k = k << 1)
			{
				int sumReg[D_HOST_MAX_RADIX_SIZE];
				for (int threadId = 0; threadId < radixStride; ++threadId)
				{
					int a = radixPrefixScan[radixStride / 2 + threadId];
					int b = radixPrefixScan[radixStride / 2 + threadId - k];
					sumReg[threadId] = a + b;
				}
				for (int threadId = 0; threadId < radixStride; ++threadId)
				{
					radixPrefixScan[radixStride / 2 + threadId] = sumReg[threadId];
				}
			}
#endif


			//for (int k = 1; k < blockStride; k = k << 1)
			//{
			//	for (int j = k; j > 0; j = j >> 1)
			//	{
			//		int highMask = -j;
			//		int lowMask = ~highMask;
			//		for (int threadId = 0; threadId < blockStride / 2; ++threadId)
			//		{
			//			int lowIndex = threadId & lowMask;
			//			int highIndex = (threadId & highMask) * 2;
			//
			//			int id0 = highIndex + lowIndex;
			//			int id1 = highIndex + lowIndex + j;
			//			int oddEven = highIndex & k * 2;
			//
			//			int a = sortedRadix[id0];
			//			int b = sortedRadix[id1];
			//
			//			int test = a < b;
			//			int a1 = test ? a : b;
			//			int b1 = test ? b : a;
			//
			//			int a2 = oddEven ? b1 : a1;
			//			int b2 = oddEven ? a1 : b1;
			//
			//			sortedRadix[id0] = a2;
			//			sortedRadix[id1] = b2;
			//		}
			//	}
			//}
			//
			//for (int threadId = 0; threadId < blockStride; ++threadId)
			//{
			//	int index = bashSize + threadId;
			//	if (index < size)
			//	{
			//		int keyValue = sortedRadix[threadId];
			//		int keyHigh = keyValue >> 16;
			//		int keyLow = keyValue & 0xffff;
			//		int dstOffset1 = radixPrefixStart[keyHigh];
			//		int dstOffset0 = threadId - radixPrefixScan[radixStride / 2 + keyHigh];
			//		dst[dstOffset0 + dstOffset1] = cachedItems[keyLow];
			//	}
			//}
			//
			//for (int threadId = 0; threadId < radixStride; ++threadId)
			//{
			//	radixPrefixStart[threadId] += radixPrefixCount[threadId];
			//}

			bashSize += blockStride;
		}
	};

#elif defined (D_USE_CPU_LOCAL_COUNTING_SORT)
	auto MergeBuckects = [&](int blockIdx, int blocksCount, int computeUnits)
	{
		T cachedItems[D_HOST_SORT_BLOCK_SIZE];
		int sortedRadix[D_HOST_SORT_BLOCK_SIZE];
		int dstLocalOffset[D_HOST_SORT_BLOCK_SIZE];
		int radixPrefixCount[D_HOST_MAX_RADIX_SIZE];
		int radixPrefixStart[D_HOST_MAX_RADIX_SIZE];
		int radixPrefixScan[D_HOST_SORT_BLOCK_SIZE / 2 + D_HOST_SORT_BLOCK_SIZE + 1];

		int size = src.GetCount();
		int radixStride = (1 << exponentRadix);
		int blockStride = D_HOST_SORT_BLOCK_SIZE;
		int radixBase = blockIdx * radixStride;
		int bashSize = blocksCount * blockStride * blockIdx;
		int radixPrefixOffset = computeUnits * radixStride;

		ndEvaluateKey evaluator;
		for (int threadId = 0; threadId < radixStride; ++threadId)
		{
			int a = scansBuffer[radixBase + threadId];
			int b = scansBuffer[radixPrefixOffset + threadId];
			radixPrefixStart[threadId] = a + b;

		}
		for (int threadId = 0; threadId < blockStride; ++threadId)
		{
			radixPrefixScan[threadId] = 0;
		}

		for (int i = 0; i < blocksCount; ++i)
		{
			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixCount[threadId] = 0;
			}

			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int radix = radixStride - 1;
				int index = bashSize + threadId;
				if (index < size)
				{
					cachedItems[threadId] = src[index];
					radix = evaluator.GetRadix(cachedItems[threadId]);
				}
				radixPrefixCount[radix] ++;
				sortedRadix[threadId] = (threadId << 16) + radix;
				radixPrefixScan[threadId] = 0;
			}

			int bit = 1;
			for (int j = blockStride; j; j = j >> 1)
			{
				int keyReg[D_HOST_SORT_BLOCK_SIZE];
				for (int threadId = 0; threadId < blockStride; ++threadId)
				{
					keyReg[threadId] = sortedRadix[threadId];
				}

				for (int threadId = 0; threadId < blockStride; ++threadId)
				{
					int test = keyReg[threadId] & bit;
					int mask = test ? 1 : 0;
					dstLocalOffset[threadId] = mask;
					radixPrefixScan[blockStride / 2 + threadId + 1] = mask ? 1 << 16 : 1;
				}

				for (int k = 1; k < blockStride; k = k << 1)
				{
					int sumReg[D_HOST_SORT_BLOCK_SIZE];
					for (int threadId = 0; threadId < blockStride; ++threadId)
					{
						int a = radixPrefixScan[blockStride / 2 + threadId];
						int b = radixPrefixScan[blockStride / 2 + threadId - k];
						sumReg[threadId] = a + b;
					}
					for (int threadId = 0; threadId < blockStride; ++threadId)
					{
						radixPrefixScan[blockStride / 2 + threadId] = sumReg[threadId];
					}
				}

				int base = (radixPrefixScan[blockStride / 2 + blockStride] + radixPrefixScan[blockStride / 2 + blockStride - 1]) << 16;
				for (int threadId = 0; threadId < blockStride; ++threadId)
				{
					int a = radixPrefixScan[blockStride / 2 + threadId] & 0xffff;
					int b = (radixPrefixScan[blockStride / 2 + threadId] + base) >> 16;
					dstLocalOffset[threadId] = dstLocalOffset[threadId] ? b : a;
				}

				for (int threadId = 0; threadId < blockStride; ++threadId)
				{
					int dstIndex = dstLocalOffset[threadId];
					sortedRadix[dstIndex] = keyReg[threadId];
				}

				bit = bit << 1;
			}

			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixScan[radixStride / 2 + threadId + 1] = radixPrefixCount[threadId];
			}

			for (int k = 1; k < radixStride; k = k << 1)
			{
				int sumReg[D_HOST_MAX_RADIX_SIZE];
				for (int threadId = 0; threadId < radixStride; ++threadId)
				{
					int a = radixPrefixScan[radixStride / 2 + threadId];
					int b = radixPrefixScan[radixStride / 2 + threadId - k];
					sumReg[threadId] = a + b;
				}
				for (int threadId = 0; threadId < radixStride; ++threadId)
				{
					radixPrefixScan[radixStride / 2 + threadId] = sumReg[threadId];
				}
			}

			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int index = bashSize + threadId;
				if (index < size)
				{
					int keyValue = sortedRadix[threadId];
					int keyLow = keyValue >> 16;
					int keyHigh = keyValue & 0xffff;
					int dstOffset1 = radixPrefixStart[keyHigh];
					int dstOffset0 = threadId - radixPrefixScan[radixStride / 2 + keyHigh];
					dst[dstOffset0 + dstOffset1] = cachedItems[keyLow];
				}
			}

			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixStart[threadId] += radixPrefixCount[threadId];
			}

			bashSize += blockStride;
		}
	};

#else

	//auto MergeBuckects = [&](int blockIdx, int blocksCount, int computeUnits)
	//{
	//	int scanBaseAdress[D_HOST_MAX_RADIX_SIZE];
	//	
	//	int size = src.GetCount();
	//	int blockIndex = blockIdx;
	//	int radixStride = (1 << exponentRadix);
	//	int blockStride = D_HOST_SORT_BLOCK_SIZE;
	//	int radixBase = blockIndex * radixStride;
	//	int radixPrefixOffset = computeUnits * radixStride;
	//	int bashSize = blocksCount * blockStride * blockIdx;
	//	
	//	for (int threadId = 0; threadId < radixStride; ++threadId)
	//	{
	//		scanBaseAdress[threadId] = scansBuffer[radixPrefixOffset + threadId] + scansBuffer[radixBase + threadId];
	//	}
	//	
	//	ndEvaluateKey evaluator;
	//	for (int i = 0; i < blocksCount; ++i)
	//	{
	//		for (int threadId = 0; threadId < blockStride; ++threadId)
	//		{
	//			int index = bashSize + threadId;
	//			if (index < size)
	//			{
	//				const T item(src[index]);
	//				int radix = evaluator.GetRadix(item);
	//				int address = scanBaseAdress[radix]++;
	//				dst[address] = item;
	//			}
	//		}
	//		bashSize += blockStride;
	//	}
	//};
#endif


#define D_SORTING_ALGORITHM 1

#if (D_SORTING_ALGORITHM == 0)
	// using simple prefix scan sum
	// using a simple bitonix sort, the sort has a two ways bank conflit
	auto MergeBuckects = [&](int blockIdx, int blocksCount, int computeUnits)
	{
		T cachedItems[D_HOST_SORT_BLOCK_SIZE];
		int radixPrefixCount[D_HOST_MAX_RADIX_SIZE];
		int radixPrefixStart[D_HOST_MAX_RADIX_SIZE];
		//ndBankFreeArray<D_HOST_SORT_BLOCK_SIZE> sortedRadix;
		int sortedRadix[D_HOST_SORT_BLOCK_SIZE];
		int radixPrefixScan[D_HOST_MAX_RADIX_SIZE / 2 + D_HOST_MAX_RADIX_SIZE + 1];

		int size = src.GetCount();
		int radixStride = (1 << exponentRadix);
		int blockStride = D_HOST_SORT_BLOCK_SIZE;
		int radixBase = blockIdx * radixStride;
		int bashSize = blocksCount * blockStride * blockIdx;
		int radixPrefixOffset = computeUnits * radixStride;

		ndEvaluateKey evaluator;
		for (int threadId = 0; threadId < radixStride; ++threadId)
		{
			int a = scansBuffer[radixBase + threadId];
			int b = scansBuffer[radixPrefixOffset + threadId];
			radixPrefixStart[threadId] = a + b;
			radixPrefixScan[threadId] = 0;
		}

		for (int i = 0; i < blocksCount; ++i)
		{
			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixCount[threadId] = 0;
			}

			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int index = bashSize + threadId;
				if (index < size)
				{
					cachedItems[threadId] = src[index];
					int radix = evaluator.GetRadix(cachedItems[threadId]);
					radixPrefixCount[radix] ++;
					sortedRadix[threadId] = (radix << 16) + threadId;
				}
				else
				{
					sortedRadix[threadId] = (radixStride << 16);
				}
			}


			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixScan[radixStride / 2 + threadId + 1] = radixPrefixCount[threadId];
			}
			for (int k = 1; k < radixStride; k = k << 1)
			{
				int sumReg[D_HOST_MAX_RADIX_SIZE];
				for (int threadId = 0; threadId < radixStride; ++threadId)
				{
					int a = radixPrefixScan[radixStride / 2 + threadId];
					int b = radixPrefixScan[radixStride / 2 + threadId - k];
					sumReg[threadId] = a + b;
				}
				for (int threadId = 0; threadId < radixStride; ++threadId)
				{
					radixPrefixScan[radixStride / 2 + threadId] = sumReg[threadId];
				}
			}

			for (int k = 1; k < blockStride; k = k << 1)
			{
				for (int j = k; j > 0; j = j >> 1)
				{
					int highMask = -j;
					int lowMask = ~highMask;
					for (int threadId = 0; threadId < blockStride / 2; ++threadId)
					{
						int lowIndex = threadId & lowMask;
						int highIndex = (threadId & highMask) * 2;
			
						int id0 = highIndex + lowIndex;
						int id1 = highIndex + lowIndex + j;
						int oddEven = highIndex & k * 2;
						cuTrace(("%d ", id0 % D_BANK_COUNT));
						//cuTrace(("%d ", sortedRadix.GetBankAddress(id0) % D_BANK_COUNT));
			
						int a = sortedRadix[id0];
						int b = sortedRadix[id1];
			
						int test = a < b;
						int a1 = test ? a : b;
						int b1 = test ? b : a;
			
						int a2 = oddEven ? b1 : a1;
						int b2 = oddEven ? a1 : b1;
			
						sortedRadix[id0] = a2;
						sortedRadix[id1] = b2;
					}
					cuTrace(("\n"));
				}
				cuTrace(("\n"));
			}
			
			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int index = bashSize + threadId;
				if (index < size)
				{
					int keyValue = sortedRadix[threadId];
					int keyHigh = keyValue >> 16;
					int keyLow = keyValue & 0xffff;
					int dstOffset1 = radixPrefixStart[keyHigh];
					int dstOffset0 = threadId - radixPrefixScan[radixStride / 2 + keyHigh];
					dst[dstOffset0 + dstOffset1] = cachedItems[keyLow];
				}
			}
			
			for (int threadId = 0; threadId < radixStride; ++threadId)
			{
				radixPrefixStart[threadId] += radixPrefixCount[threadId];
			}

			bashSize += blockStride;
		}
	};

#elif (D_SORTING_ALGORITHM == 1)

	auto MergeBuckects = [&](int blockIdx, int blocksCount, int computeUnits)
	{
		//T cachedItems[D_HOST_SORT_BLOCK_SIZE];
		//int radixPrefixCount[D_HOST_MAX_RADIX_SIZE];
		//int radixPrefixStart[D_HOST_MAX_RADIX_SIZE];
		//ndBankFreeArray<D_HOST_SORT_BLOCK_SIZE> sortedRadix;
		//ndBankFreeArray<D_HOST_MAX_RADIX_SIZE> radixPrefixScan;

		int sortedRadix[D_HOST_SORT_BLOCK_SIZE];
		int dstLocalOffset[D_HOST_SORT_BLOCK_SIZE];
		int radixPrefixScan[2 * (D_HOST_SORT_BLOCK_SIZE + 1)];
		//ndBankFreeArray<D_HOST_MAX_RADIX_SIZE> radixPrefixScan;

		auto ShufleUp = [&](const int* in, int* out, int offset)
		{
			for (int i = 0; i < offset; ++i)
			{
				out[i] = in[i];
			}

			for (int i = D_BANK_COUNT - offset - 1; i >= 0; --i)
			{
				out[i + offset] = in[i];
			}
		};

		int size = src.GetCount();
		int radixStride = (1 << exponentRadix);
		int blockStride = D_HOST_SORT_BLOCK_SIZE;
		int radixBase = blockIdx * radixStride;
		int bashSize = blocksCount * blockStride * blockIdx;
		int radixPrefixOffset = computeUnits * radixStride;

		ndEvaluateKey evaluator;
		//for (int threadId = 0; threadId < radixStride; ++threadId)
		//{
		//	int a = scansBuffer[radixBase + threadId];
		//	int b = scansBuffer[radixPrefixOffset + threadId];
		//	radixPrefixStart[threadId] = a + b;
		//	radixPrefixScan[threadId] = 0;
		//}

		radixPrefixScan[0] = 0;
		radixPrefixScan[D_HOST_SORT_BLOCK_SIZE + 1] = 0;

		for (int i = 0; i < blocksCount; ++i)
		{
			//for (int threadId = 0; threadId < radixStride; ++threadId)
			//{
			//	radixPrefixCount[threadId] = 0;
			//}

			for (int threadId = 0; threadId < blockStride; ++threadId)
			{
				int index = bashSize + threadId;
				//int sortKey = radixStride << 16;
				int sortKey = radixStride;
				if (index < size)
				{
					const T item(src[index]);
					//cachedItems[threadId] = src[index];
					int radix = evaluator.GetRadix(item);
					//radixPrefixCount[radix] ++;
					//sortKey = (radix << 16) + threadId;
					sortKey = radix;
				}
				sortedRadix[threadId] = sortKey;
			}

			for (int bit = 0; (1<<(bit * 2)) < radixStride; ++bit)
			{
				int keyReg[D_HOST_SORT_BLOCK_SIZE];
				for (int threadId = 0; threadId < D_HOST_SORT_BLOCK_SIZE; ++threadId)
				{
					keyReg[threadId] = sortedRadix[threadId];
				}

				int radixPrefixScanReg0[D_HOST_SORT_BLOCK_SIZE];
				int radixPrefixScanReg1[D_HOST_SORT_BLOCK_SIZE];
				for (int threadId = 0; threadId < D_HOST_SORT_BLOCK_SIZE; ++threadId)
				{
					int test = (keyReg[threadId] >> (bit * 2)) & 0x3;
					dstLocalOffset[threadId] = test;
					//radixPrefixScanReg[threadId] = 1 << (test << 3);
					int bit0 = (test == 0) ? 1 : 0;
					int bit1 = (test == 1) ? 1 << 16 :  0;
					int bit2 = (test == 2) ? 1 : 0;
					int bit3 = (test == 3) ? 1 << 16 : 0;
					radixPrefixScanReg0[threadId] = bit0 + bit1;
					radixPrefixScanReg1[threadId] = bit2 + bit3;
				}

				// sync free loop
				for (int bankBase = 0; bankBase < D_HOST_SORT_BLOCK_SIZE; bankBase += D_BANK_COUNT)
				{
					for (int n = 1; n < D_BANK_COUNT; n *= 2)
					{
						//int radixPrefixScanReg1[D_BANK_COUNT];
						int radixPrefixScanRegTemp0[D_HOST_SORT_BLOCK_SIZE];
						int radixPrefixScanRegTemp1[D_HOST_SORT_BLOCK_SIZE];
						ShufleUp(&radixPrefixScanReg0[bankBase], &radixPrefixScanRegTemp0[bankBase], n);
						ShufleUp(&radixPrefixScanReg1[bankBase], &radixPrefixScanRegTemp1[bankBase], n);
						for (int threadId = 0; threadId < D_BANK_COUNT; ++threadId)
						{
							if ((threadId & (D_BANK_COUNT - 1)) >= n)
							{
								radixPrefixScanReg0[bankBase + threadId] += radixPrefixScanRegTemp0[bankBase + threadId];
								radixPrefixScanReg1[bankBase + threadId] += radixPrefixScanRegTemp1[bankBase + threadId];
							}
						}
					}
				}
				// write to memory add finish the prefix scan
				for (int threadId = 0; threadId < D_HOST_SORT_BLOCK_SIZE; ++threadId)
				{
					radixPrefixScan[threadId + 1] = radixPrefixScanReg0[threadId];
					radixPrefixScan[threadId + 1 + D_HOST_SORT_BLOCK_SIZE + 1] = radixPrefixScanReg1[threadId];
				}

				int scale = 0;
				for (int segment = blockStride; segment > D_BANK_COUNT; segment >>= 1)
				{
					for (int threadId = 0; threadId < blockStride / 2; ++threadId)
					//for (int threadId = 0; threadId < blockStride / 2; threadId += D_BANK_COUNT)
					{
						int baseBank = threadId >> (D_LOG_BANK_COUNT + scale);
						int baseIndex = (baseBank << (D_LOG_BANK_COUNT + scale + 1)) + (1 << (D_LOG_BANK_COUNT + scale)) + 1 - 1;
						int bankIndex = threadId & ((1 << (D_LOG_BANK_COUNT + scale)) - 1);
						int scanIndex = baseIndex + bankIndex + 1;

						int base0 = radixPrefixScan[baseIndex];
						int base1 = radixPrefixScan[D_HOST_SORT_BLOCK_SIZE + 1 + baseIndex];
						radixPrefixScan[scanIndex] += base0;
						radixPrefixScan[D_HOST_SORT_BLOCK_SIZE + 1 + scanIndex] += base1;
					}
					scale ++;
				}
				
				int base0 = radixPrefixScan[1 * (D_HOST_SORT_BLOCK_SIZE + 1) - 1];
				int base1 = radixPrefixScan[2 * (D_HOST_SORT_BLOCK_SIZE + 1) - 1];
				base0 = base0 + (base0 << 16);
				int val = (base0 & 0xffff0000) + (base0 >> 16);
				base1 = base1 + (base1 << 16) + val;
				base1 = (base1 << 16) + (base0 >> 16);
				base0 = (base0 << 16);
				//for (int threadId = D_BANK_COUNT - 1; threadId >= 0; --threadId)
				//{
				//	radixPrefixScanReg0[threadId + 1] = radixPrefixScanReg0[threadId];
				//	radixPrefixScanReg1[threadId + 1] = radixPrefixScanReg1[threadId];
				//}
				//radixPrefixScanReg0[0] = 0;
				
				for (int threadId = 0; threadId < D_HOST_SORT_BLOCK_SIZE; ++threadId)
				{
				//	int key = radixPrefixScanReg[threadId];
				//	int shift = dstLocalOffset[threadId] << 3;
				//	int keyIndex = ((key + base) >> shift) & 0xff;
				//	dstLocalOffset[threadId] = keyIndex;
				}
				
				//for (int threadId = 0; threadId < D_BANK_COUNT; ++threadId)
				//{
				//	int dstIndex = dstLocalOffset[threadId];
				//	sortedRadix[bank + dstIndex] = keyReg[threadId];
				//}
				cuTrace(("\n"));
			}

			cuTrace(("\n"));

			//for (int threadId = 0; threadId < blockStride; ++threadId)
			//{
			//	int index = bashSize + threadId;
			//	if (index < size)
			//	{
			//		int keyValue = sortedRadix[threadId];
			//		int keyHigh = keyValue >> 16;
			//		int keyLow = keyValue & 0xffff;
			//		int dstOffset1 = radixPrefixStart[keyHigh];
			//		int dstOffset0 = threadId - radixPrefixScan[radixStride / 2 + keyHigh];
			//		dst[dstOffset0 + dstOffset1] = cachedItems[keyLow];
			//	}
			//}
			//
			//for (int threadId = 0; threadId < radixStride; ++threadId)
			//{
			//	radixPrefixStart[threadId] += radixPrefixCount[threadId];
			//}

			bashSize += blockStride;
		}
	};

#else
	#error implement new local sort and merge algorthm?
#endif

	ndAssert(src.GetCount() == dst.GetCount());
	ndAssert((1 << exponentRadix) <= D_HOST_MAX_RADIX_SIZE);

	//int deviceComputeUnits = 20;
	int deviceComputeUnits = 1;
	int itemCount = src.GetCount();
	int computeUnitsBashCount = (itemCount + D_HOST_SORT_BLOCK_SIZE - 1) / D_HOST_SORT_BLOCK_SIZE;
	int bashCount = (computeUnitsBashCount + deviceComputeUnits - 1) / deviceComputeUnits;
	int computeUnits = (itemCount + bashCount * D_HOST_SORT_BLOCK_SIZE - 1) / (bashCount * D_HOST_SORT_BLOCK_SIZE);
	ndAssert(computeUnits <= deviceComputeUnits);

	for (int block = 0; block < computeUnits; ++block)
	{
		CountItems(block, bashCount);
	}

	for (int block = 0; block < 1; ++block)
	{
		AddPrefix(block, 1 << exponentRadix, computeUnits);
	}

	for (int block = 0; block < computeUnits; ++block)
	{
		MergeBuckects(block, bashCount, computeUnits);
	}
}

#endif