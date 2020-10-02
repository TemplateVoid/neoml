/* Copyright © 2017-2020 ABBYY Production LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
--------------------------------------------------------------------------------------------------------------*/

#pragma once

#include <NeoMathEngine/NeoMathEngineDefs.h>

#ifdef NEOML_USE_VULKAN

#include <NeoMathEngine/CrtAllocatedObject.h>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <MathEngineAllocator.h>
#include <VulkanDll.h>

namespace NeoML {

// The maximum number of bindings per shader
constexpr int VulkanMaxBindingCount = 8;

// All available shaders
enum TShader {
	SH_VectorFillScalar,
	SH_Transpose,
	SH_BlobConvolution,
	SH_BlobConvolutionAdreno,
	SH_BlobConvolution8,
	SH_BlobConvolution8Adreno,
	SH_BlobConvolutionBackward,
	SH_BlobConvolutionBackwardAdreno,
	SH_BlobConvolutionLearnAdd,
	SH_PrepareFilterForConvolutionBackwardAdreno,
	SH_VectorELU,
	SH_VectorELUDiff,
	SH_VectorELUDiffOp,
	SH_VectorReLU,
	SH_VectorReLU4,
	SH_VectorReLUDiff,
	SH_VectorLeakyReLU,
	SH_VectorLeakyReLUDiff,
	SH_VectorHSwish,
	SH_VectorHSwishDiff,
	SH_VectorEltwiseMax,
	SH_VectorEltwiseMin,
	SH_VectorAbs,
	SH_VectorAbsDiff,
	SH_VectorHinge,
	SH_VectorHingeDiff,
	SH_VectorSquaredHinge,
	SH_VectorSquaredHingeDiff,
	SH_VectorHuber,
	SH_VectorHardTanh,
	SH_VectorHardTanhDiff,
	SH_VectorHardSigmoid,
	SH_VectorHardSigmoidDiff,
	SH_VectorHardSigmoidDiffOp,
	SH_VectorExp,
	SH_VectorLog,
	SH_VectorBernulliKLDerivative,
	SH_VectorFillBernoulli,
	SH_BlobMaxPooling,
	SH_BlobMeanPooling,
	SH_PrepareBlobForConvolution,
	SH_PrepareBlobForConvolutionAdreno,
	SH_PrepareBlobWithPaddingBuffers,
	SH_PrepareBlobWithPaddingAdreno,
	SH_PrepareFilter3x3ForConvolutionAdreno,
	SH_BlobConvolution3x3s1d1,
	SH_BlobConvolution3x3s1d1Adreno,
	SH_MultiplyMatrixByMatrix,
	SH_BatchMultiplyMatrixByMatrixBorders,
	SH_BatchMultiplyMatrixByTransposedMatrix,
	SH_BatchMultiplyMatrixByTransposedMatrixBorders,
	SH_BatchMultiplyTransposedMatrixByMatrix,
	SH_BatchMultiplyTransposedMatrixByMatrixBorders,
	SH_BatchInitAddMultiplyMatrixByTransposedMatrix,
	SH_BatchInitMultiplyMatrixByTransposedMatrixBorders,
	SH_Matrix2InterleavedAdreno,
	SH_MultiplyMatrixInterleavedAdreno,
	SH_MultiplyMatrixInterleavedBoardersAdreno,
	SH_MultiplyMatrixByDiagMatrixAdreno,
	SH_MultiplyMatrixByDiagMatrix,
	SH_MultiplyDiagMatrixByMatrixAdreno,
	SH_MultiplyDiagMatrixByMatrix,
	SH_MultiplyDiagMatrixByMatrixAndAdd,
	SH_MultiplySparseMatrixByTransposedMatrix,
	SH_MultiplyTransposedMatrixBySparseMatrix,
	SH_AddVectorToMatrixRowsAdreno,
	SH_SetVectorToMatrixRowsAdreno,
	SH_SetVectorToMatrixRows,
	SH_SumMatrixRows,
	SH_SumMatrixColumns,
	SH_Blob3dConvolution,
	SH_Blob3dConvolutionBackward,
	SH_BlobChannelwiseConvolutionAdreno,
	SH_BlobChannelwiseConvolution,
	SH_BlobChannelwiseConvolution3x3s1,
	SH_BlobChannelwiseConvolution3x3s2,
	SH_VectorAddFloat4,
	SH_VectorAddFloat1,
	SH_VectorAddValue,
	SH_VectorAddInt,
	SH_VectorSub,
	SH_VectorMultiply,
	SH_VectorMultiplyAndAdd,
	SH_VectorMultiplyAndSub,
	SH_VectorEltwiseDivide,
	SH_VectorEltwisePower,
	SH_VectorSqrt,
	SH_VectorInv,
	SH_VectorMinMax,
	SH_VectorSigmoid,
	SH_VectorSigmoidDiff,
	SH_VectorSigmoidDiffOp,
	SH_VectorTanh,
	SH_VectorTanhDiff,
	SH_VectorTanhDiffOp,
	SH_VectorPower,
	SH_VectorPowerDiff,
	SH_VectorPowerDiffOp,
	SH_VectorL1DiffAdd,
	SH_VectorDotProduct,
	SH_VectorEltwiseLogSumExp,
	SH_VectorSum,
	SH_RowMultiplyMatrixByMatrix,
	SH_VectorEqual,
	SH_VectorToImage,
	SH_SetVectorToMatrixElements,
	SH_LookupAndSum,
	SH_Upsampling2DForward,
	SH_Blob3dMaxPoolingNoIndices,
	SH_Blob3dMeanPooling,
	SH_BlobMaxOverTimePoolingNoIndices,
	SH_FindMaxValueInRows,
	SH_FindMaxValueInRowsNoIndices,
	SH_BatchFindMaxValueInColumns,
	SH_BlobGlobalMaxPooling,
	SH_AddMatrixElementsToVector,
	SH_AddMatrixElementsToVectorEx,
	SH_AddVectorToMatrixColumnsInt,
	SH_AddVectorToMatrixColumnsFloatAdreno,
	SH_AddVectorToMatrixColumnsFloat,
	SH_BatchAddVectorToMatrixRows,
	SH_EnumBinarizationFloat,
	SH_EnumBinarizationInt,
	SH_BitSetBinarization,
	SH_BlobResizeImage,
	SH_MatrixLogSumExpByRows,
	SH_MatrixSoftmaxByRows,
	SH_MatrixSoftmaxByColumns,
	SH_BlobSpatialDropout,
	SH_BuildIntegerHist,
	SH_VectorFindMaxValueInSetNoIndices,
	SH_VectorFindMaxValueInSet,
	SH_MatrixSpreadRowsFloat,
	SH_MatrixSpreadRowsFloatAdd,
	SH_MatrixSpreadRowsInt,
	SH_FindMaxValueInColumns,
	SH_FindMaxValueInColumnsNoIndices,
	SH_FindMinValueInColumns,
	SH_BlobGetSubSequence,
	SH_BlobGetSubSequenceNoIndices,
	SH_BlobConvertFromRLE,
	SH_BlobSplitByDim,
	SH_BlobMergeByDim,
	SH_VectorMultichannelLookupAndCopyFloat,
	SH_VectorMultichannelCopyFloat,
	SH_VectorMultichannelLookupAndCopyInt,
	SH_VectorMultichannelCopyInt,
	SH_BlobTimeConvolutionPrepare,
	SH_BlobReorgFloat,
	SH_BlobReorgInt,

	SH_Count
};

struct CWorkGroup
{
	static constexpr uint16_t Size = 64;

	uint16_t X;
	uint16_t Y;
	uint16_t Z;

	CWorkGroup() : X(Size), Y(1), Z(1) {}
};

inline bool operator==(const CWorkGroup& lhs, const CWorkGroup& rhs) 
{
	return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
}
}

namespace std {
	template <>
	struct hash<NeoML::CWorkGroup> {
		std::size_t operator()(const NeoML::CWorkGroup& workGroup) const {
			uint64_t result = (static_cast<uint64_t>(workGroup.X) << 32) | (workGroup.Y << 16) | workGroup.Z;
			return std::hash<uint64_t>{}(result);
		}
	};
}

namespace NeoML {
//------------------------------------------------------------------------------------------------------------
// The data that describes a shader
struct CVulkanShaderData : public CCrtAllocatedObject {
	VkShaderModule Module;
	VkDescriptorSetLayout DescLayout;
	VkPipelineLayout Layout;
	bool IsImageBased;
	std::unordered_map<CWorkGroup, VkPipeline> pipelines;
	
	CVulkanShaderData();
};

inline CVulkanShaderData::CVulkanShaderData() :
	Module(VK_NULL_HANDLE),
	DescLayout(VK_NULL_HANDLE),
	Layout(VK_NULL_HANDLE),
	IsImageBased(false)
{
}

struct CVulkanShader
{
	CVulkanShader( const CVulkanShaderData& data ) :
		Module(data.Module),
		DescLayout(data.DescLayout),
		Layout(data.Layout),
		IsImageBased(data.IsImageBased)
	{}
	
	constexpr int GetGroupSize() const { return CWorkGroup::Size; }

	VkShaderModule Module;
	VkDescriptorSetLayout DescLayout;
	VkPipelineLayout Layout;
	bool IsImageBased;
	VkPipeline Pipeline;
	int GroupSizeX;
	int GroupSizeY;
	int GroupSizeZ;
};
//------------------------------------------------------------------------------------------------------------

class CVulkanDll;

// The shader loader
class CVulkanShaderLoader : public CCrtAllocatedObject {
public:
	explicit CVulkanShaderLoader( const CVulkanDevice& vulkanDevice );
	~CVulkanShaderLoader();

	// Gets the shader data
	CVulkanShader GetShaderData(TShader id, bool isIB,
		const uint32_t* code, int codeLen, size_t paramSize, 
		int imageCount, int samplerCount, int bufferCount, int dimensions,
		int gridWidth, int gridHeight, int gridDepth );
private:
	void calculateThreadGroupSize( int dimensions, uint16_t& threadGroupSizeX, uint16_t& threadGroupSizeY, uint16_t& threadGroupSizeZ,
		int gridWidth, int gridHeight, int gridDepth ) const;

	const CVulkanDevice& device;
	std::vector< CVulkanShaderData*, CrtAllocator<CVulkanShaderData*> > shaders; // cache
};

// A helper macro
#define GET_SHADER_DATA(shader, hasParam, imageCount, samplerCount, bufferCount, countX, countY, countZ)					\
	GetShaderData(SH_##shader, shader##IsIB, Shader_##shader, sizeof(Shader_##shader),				\
		(hasParam ? sizeof(PARAM_STRUCT(shader)) : 0), (imageCount), (samplerCount), (bufferCount),	\
		shader##Dimensions, (countX), (countY), (countZ) )
}

#endif // NEOML_USE_VULKAN
