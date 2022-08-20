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

#include "ndDeepBrainStdafx.h"
#include "ndDeepBrain.h"
#include "ndDeepBrainLayer.h"
#include "ndDeepBrainParallelGradientDescendTrainingOperator.h"

class ndDeepBrainParallelGradientDescendTrainingOperator::ndGradientChannel : public ndDeepBrainTrainingOperator
{
	public:
	ndGradientChannel(ndDeepBrainParallelGradientDescendTrainingOperator* const owner)
		:ndDeepBrainTrainingOperator(owner->m_instance.GetBrain())
		,m_output(owner->m_output)
		,m_zDerivative(owner->m_zDerivative)
		,m_biasGradients(owner->m_biasGradients)
		,m_weightGradients(owner->m_weightGradients)
		,m_biasGradientsAcc(owner->m_biasGradients)
		,m_weightGradientsAcc(owner->m_weightGradients)
		,m_owner(owner)
		,m_averageError(0.0f)
	{
	}

	virtual ~ndGradientChannel()
	{
	}

	virtual void Optimize(const ndDeepBrainMatrix&, const ndDeepBrainMatrix&, ndReal, ndInt32)
	{
		ndAssert(0);
	}

	void MakePrediction(const ndDeepBrainVector& input)
	{
		m_instance.MakePrediction(input, m_output);
		const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());

		const ndDeepBrainVector& output = m_instance.GetOutPut();
		const ndDeepBrainPrefixScan& zPrefixScan = m_instance.GetPrefixScan();
		for (ndInt32 i = layers.GetCount() - 1; i >= 0; --i)
		{
			ndDeepBrainLayer* const layer = layers[i];
			const ndDeepBrainMemVector z(&output[zPrefixScan[i + 1]], layer->GetOuputSize());
			ndDeepBrainMemVector zDerivative(&m_zDerivative[zPrefixScan[i + 1]], layer->GetOuputSize());
			layer->ActivationDerivative(z, zDerivative);
		}
	}

	void BackPropagateOutputLayer(const ndDeepBrainVector& groundTruth)
	{
		const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
		const ndInt32 layerIndex = layers.GetCount() - 1;

		const ndDeepBrainPrefixScan& zPrefixScan = m_instance.GetPrefixScan();
		ndDeepBrainLayer* const ouputLayer = layers[layerIndex];
		const ndInt32 inputCount = ouputLayer->GetInputSize();
		const ndInt32 outputCount = ouputLayer->GetOuputSize();

		const ndDeepBrainVector& output = m_instance.GetOutPut();
		ndDeepBrainMemVector biasGradients(&m_biasGradients[zPrefixScan[layerIndex + 1]], outputCount);
		ndDeepBrainMemVector biasGradientsAcc(&m_biasGradientsAcc[zPrefixScan[layerIndex + 1]], outputCount);
		const ndDeepBrainMemVector z(&output[zPrefixScan[layerIndex + 1]], outputCount);
		const ndDeepBrainMemVector zDerivative(&m_zDerivative[zPrefixScan[layerIndex + 1]], outputCount);

		biasGradients.Sub(z, groundTruth);
		biasGradients.Mul(biasGradients, zDerivative);
		biasGradientsAcc.Add(biasGradientsAcc, biasGradients);

		const ndInt32 stride = (inputCount + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
		ndReal* weightGradientPtr = &m_weightGradients[m_owner->m_weightGradientsPrefixScan[layerIndex]];
		ndReal* weightGradientPtrAcc = &m_weightGradientsAcc[m_owner->m_weightGradientsPrefixScan[layerIndex]];
		const ndDeepBrainMemVector z0(&output[zPrefixScan[layerIndex]], inputCount);
		for (ndInt32 i = 0; i < outputCount; ++i)
		{
			ndDeepBrainMemVector weightGradient(weightGradientPtr, inputCount);
			ndDeepBrainMemVector weightGradientAcc(weightGradientPtrAcc, inputCount);
			ndFloat32 gValue = biasGradients[i];
			weightGradient.ScaleSet(z0, gValue);
			weightGradientAcc.Add(weightGradientAcc, weightGradient);
			weightGradientPtr += stride;
		}
	}

	void BackPropagateCalculateBiasGradient(ndInt32 layerIndex)
	{
		const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
		ndDeepBrainLayer* const layer = layers[layerIndex + 1];

		const ndDeepBrainPrefixScan& zPrefixScan = m_instance.GetPrefixScan();

		const ndDeepBrainMemVector biasGradients1(&m_biasGradients[zPrefixScan[layerIndex + 2]], layer->GetOuputSize());
		ndDeepBrainMemVector biasGradients(&m_biasGradients[zPrefixScan[layerIndex + 1]], layer->GetInputSize());
		ndDeepBrainMemVector biasGradientsAcc(&m_biasGradientsAcc[zPrefixScan[layerIndex + 1]], layer->GetInputSize());
		const ndDeepBrainMatrix& matrix = *m_owner->m_weightsLayersTranspose[layerIndex + 1];
		const ndDeepBrainMemVector zDerivative(&m_zDerivative[zPrefixScan[layerIndex + 1]], layer->GetInputSize());

		matrix.Mul(biasGradients1, biasGradients);
		biasGradients.Mul(biasGradients, zDerivative);
		biasGradientsAcc.Add(biasGradientsAcc, biasGradients);
	}

	void BackPropagateHiddenLayer(ndInt32 layerIndex)
	{
		const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
		BackPropagateCalculateBiasGradient(layerIndex);

		ndDeepBrainLayer* const layer = layers[layerIndex];
		const ndDeepBrainPrefixScan& zPrefixScan = m_instance.GetPrefixScan();
		const ndDeepBrainMemVector biasGradients(&m_biasGradients[zPrefixScan[layerIndex + 1]], layer->GetOuputSize());

		const ndInt32 inputCount = layer->GetInputSize();
		const ndInt32 stride = (inputCount + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
		ndReal* weightGradientPtr = &m_weightGradients[m_owner->m_weightGradientsPrefixScan[layerIndex]];
		ndReal* weightGradientPtrAcc = &m_weightGradientsAcc[m_owner->m_weightGradientsPrefixScan[layerIndex]];

		const ndDeepBrainMemVector z0(&m_instance.GetOutPut()[zPrefixScan[layerIndex]], inputCount);
		for (ndInt32 i = 0; i < layer->GetOuputSize(); ++i)
		{
			ndDeepBrainMemVector weightGradient(weightGradientPtr, inputCount);
			ndDeepBrainMemVector weightGradientAcc(weightGradientPtrAcc, inputCount);
			ndFloat32 gValue = biasGradients[i];
			weightGradient.ScaleSet(z0, gValue);
			weightGradientAcc.Add(weightGradientAcc, weightGradient);
			weightGradientPtr += stride;
		}
	}

	void BackPropagate(const ndDeepBrainVector& groundTruth)
	{
		BackPropagateOutputLayer(groundTruth);

		const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
		for (ndInt32 i = layers.GetCount() - 2; i >= 0; --i)
		{
			BackPropagateHiddenLayer(i);
		}
	}

	ndDeepBrainVector m_output;
	ndDeepBrainVector m_zDerivative;
	ndDeepBrainVector m_biasGradients;
	ndDeepBrainVector m_weightGradients;
	ndDeepBrainVector m_biasGradientsAcc;
	ndDeepBrainVector m_weightGradientsAcc;
	ndDeepBrainParallelGradientDescendTrainingOperator* m_owner;
	ndFloat32 m_averageError;
};

ndDeepBrainParallelGradientDescendTrainingOperator::ndDeepBrainParallelGradientDescendTrainingOperator(ndDeepBrain* const brain, ndInt32 threads)
	:ndDeepBrainGradientDescendTrainingOperator(brain)
	,ndThreadPool("neuralNet")
	,m_inputBatch(nullptr)
	,m_groundTruth(nullptr)
	,m_learnRate(0.0f)
	,m_steps(0)
{
	SetThreadCount(threads);
}

ndDeepBrainParallelGradientDescendTrainingOperator::~ndDeepBrainParallelGradientDescendTrainingOperator()
{
	Finish();
	for (ndInt32 i = 0; i < m_subBatch.GetCount(); ++i)
	{
		delete m_subBatch[i];
	}
}

void ndDeepBrainParallelGradientDescendTrainingOperator::SetThreadCount(ndInt32 threads)
{
	threads = ndMin(threads, D_MAX_THREADS_COUNT);
	ndThreadPool::SetThreadCount(threads);

	if (threads != m_subBatch.GetCount())
	{
		for (ndInt32 i = 0; i < m_subBatch.GetCount(); ++i)
		{
			delete m_subBatch[i];
		}
		m_subBatch.SetCount(0);

		for (ndInt32 i = 0; i < threads; ++i)
		{
			m_subBatch.PushBack(new ndGradientChannel(this));
		}
	}
}

void ndDeepBrainParallelGradientDescendTrainingOperator::ThreadFunction()
{
	Begin();
	Optimize();
	End();
}

void ndDeepBrainParallelGradientDescendTrainingOperator::Optimize(const ndDeepBrainMatrix& inputBatch, const ndDeepBrainMatrix& groundTruth, ndReal learnRate, ndInt32 steps)
{
	m_inputBatch = &inputBatch;
	m_groundTruth = &groundTruth;
	m_learnRate = learnRate;
	m_steps = steps;
	TickOne();
	Sync();
}

void ndDeepBrainParallelGradientDescendTrainingOperator::Optimize()
{
	ndAssert(m_inputBatch->GetCount() == m_groundTruth->GetCount());
	ndAssert(m_output.GetCount() == (*m_groundTruth)[0].GetCount());

	ndInt32 index = 0;
	ndInt32 batchCount = (m_inputBatch->GetCount() + m_miniBatchSize - 1) / m_miniBatchSize;
	for (ndInt32 i = 0; i < m_steps; ++i)
	{
		auto OptimizeMiniBatch = ndMakeObject::ndFunction([this, index, batchCount](ndInt32 threadIndex, ndInt32 threadCount)
		{
			const ndInt32 batchStart = index * m_miniBatchSize;
			const ndInt32 batchSize = index != (batchCount - 1) ? m_miniBatchSize : m_inputBatch->GetCount() - batchStart;

			ndGradientChannel* const channel = m_subBatch[threadIndex];
			channel->m_biasGradientsAcc.Set(0.0f);
			channel->m_weightGradientsAcc.Set(0.0f);

			channel->m_averageError = 0.0f;
			const ndStartEnd startEnd(batchSize, threadIndex, threadCount);
			for (ndInt32 i = startEnd.m_start; i < startEnd.m_end; ++i)
			{
				const ndDeepBrainVector& input = (*m_inputBatch)[batchStart + i];
				const ndDeepBrainVector& truth = (*m_groundTruth)[batchStart + i];
				channel->MakePrediction(input);
				channel->BackPropagate(truth);
				ndFloat32 error = channel->CalculateMeanSquareError(truth);
				channel->m_averageError += error;
			}
		});
		ParallelExecute(OptimizeMiniBatch);

		auto AddPartialBiasGradients = ndMakeObject::ndFunction([this, batchCount](ndInt32 threadIndex, ndInt32 threadCount)
		{
			const ndStartEnd startEnd(m_biasGradients.GetCount(), threadIndex, threadCount);
			ndDeepBrainMemVector biasGradient(&m_biasGradients[startEnd.m_start], startEnd.m_end - startEnd.m_start);
			biasGradient.Set(0.0f);
			for (ndInt32 i = 0; i < threadCount; ++i)
			{
				ndGradientChannel* const channel = m_subBatch[threadIndex];
				const ndDeepBrainMemVector partialBiasGradient(&channel->m_biasGradientsAcc[startEnd.m_start], startEnd.m_end - startEnd.m_start);
				biasGradient.Add(biasGradient, partialBiasGradient);
			}
			biasGradient.ScaleAdd(biasGradient, 1.0f / m_miniBatchSize);
		});
		ParallelExecute(AddPartialBiasGradients);

		auto AddPartialWeightGradients = ndMakeObject::ndFunction([this, batchCount](ndInt32 threadIndex, ndInt32 threadCount)
		{
			const ndStartEnd startEnd(m_weightGradients.GetCount(), threadIndex, threadCount);
			ndDeepBrainMemVector weightGradient(&m_weightGradients[startEnd.m_start], startEnd.m_end - startEnd.m_start);
			weightGradient.Set(0.0f);
			for (ndInt32 i = 0; i < threadCount; ++i)
			{
				ndGradientChannel* const channel = m_subBatch[threadIndex];
				const ndDeepBrainMemVector partialBiasGradient(&channel->m_weightGradientsAcc[startEnd.m_start], startEnd.m_end - startEnd.m_start);
				weightGradient.Add(weightGradient, partialBiasGradient);
			}
			weightGradient.ScaleAdd(weightGradient, 1.0f / m_miniBatchSize);
		});
		ParallelExecute(AddPartialWeightGradients);

		UpdateWeights(m_learnRate);
		ApplyWeightTranspose();
		ndFloat32 error = 0.0f;
		for (ndInt32 j = 0; j < GetThreadCount(); ++j)
		{
			ndGradientChannel* const channel = m_subBatch[j];
			error += channel->m_averageError;
		}

		const ndInt32 batchStart = index * m_miniBatchSize;
		const ndInt32 batchSize = index != (batchCount - 1) ? m_miniBatchSize : m_inputBatch->GetCount() - batchStart;
		m_averageError = ndSqrt(error / batchSize);

		index = (index + 1) % batchCount;

		ndExpandTraceMessage("%f\n", m_averageError);
	}
}