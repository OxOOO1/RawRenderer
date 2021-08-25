#pragma once

#include "DrawableComponent/Drawable.h"

#include "Utilities/OutputPrint.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

class RModelImporter
{
public:

	static void Import(const std::string& fileName, RGeometryBufferCPU& mesh);//, RDrawableInstancedNodesTree& nodes);

	struct AssimpImporter
	{
		const aiScene* ReadModelAssimp(const std::string& file, bool separatePivotNodes = false, bool readMaterials = false, bool readAnimations = false, int PostProcessOps = PostProcessOpsDefault)
		{
			assimport.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, separatePivotNodes);
			assimport.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_MATERIALS, readMaterials);
			assimport.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, readAnimations);
			const auto pScene = assimport.ReadFile(file , PostProcessOps);
			assert(pScene && "Cant Import Model");
			return pScene;

		}
		Assimp::Importer assimport;

		static constexpr auto PostProcessOpsDefault = aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_GenUVCoords | aiProcess_Debone/* | aiProcess_PreTransformVertices*/; //aiProcess_ValidateDataStructure // aiProcess_OptimizeMeshes 
		static constexpr auto PostProcessSkinnedDefault = aiProcess_LimitBoneWeights | aiProcess_SplitByBoneCount | aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_GenUVCoords;

	};

};