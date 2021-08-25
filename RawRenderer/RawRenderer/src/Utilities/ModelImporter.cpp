#include "ModelImporter.h"

#include "Utilities/Types.h"

using RSimpleVertexStruct = RVertexComponentDefault::Vertex;


UINT GetNumVertices(aiMesh** meshes, UINT numMeshes)
{
	UINT numVerts = 0;
	for (int i = 0; i < numMeshes; i++)
	{
		numVerts += meshes[i]->mNumVertices;
	}
	return numVerts;
}

inline DirectX::XMMATRIX AssimpToDirectX(const aiMatrix4x4& mat)
{
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixSet(mat.a1, mat.a2, mat.a3, mat.a4, mat.b1, mat.b2, mat.b3, mat.b4, mat.c1, mat.c2, mat.c3, mat.c4, mat.d1, mat.d2, mat.d3, mat.d4));
}
/*

static RMeshDesc GMeshDesc;

void ParseNode(std::vector<RDrawableInstancedNode>& nodes, const aiNode* inNode, const aiMesh** inMeshes, int parentNodeIndex = -1)
{
	if (inNode->mName.length != 0)
	{
		UINT16 id = nodes.size();

		if (parentNodeIndex != -1)
			nodes.at(parentNodeIndex).AddChild(id);

		RDrawableInstancedNode& Node = nodes.emplace_back();

		Node.Name = inNode->mName.C_Str();
		Node.Index = id;
		Node.IndexToParent = parentNodeIndex;

		parentNodeIndex = id;

		auto m = inNode->mTransformation;
		Node.Instances.front().Transform.MatParentSpace = AssimpToDirectX(m);

		//animator->mNodeNameToNodeIDMap[nodeOut.name] = id;

		if (inNode->mNumMeshes > 0) {
			for (int i = 0; i < inNode->mNumMeshes; i++) 
			{
				auto meshIndex = inNode->mMeshes[i];
				auto* pMesh = inMeshes[meshIndex];

				GMeshDesc.NumofIndices = pMesh->mNumFaces * 3;
				GMeshDesc.NumOfVertices = pMesh->mNumVertices;

				Node.MeshDesc = GMeshDesc;

				GMeshDesc.FirstIndexPos += pMesh->mNumVertices;
				GMeshDesc.FirstVertexPos += pMesh->mNumFaces * 3;
			}
		}

	}
	else
	{
		for (size_t i = 0; i < inNode->mNumChildren; i++)
		{
			inNode->mChildren[i]->mTransformation = inNode->mTransformation;
		}
	}

	for (size_t i = 0; i < inNode->mNumChildren; i++)
	{
		ParseNode(nodes, inNode->mChildren[i], inMeshes, parentNodeIndex);
	}
}

void ParseNodes(const aiScene* pScene, RDrawableInstancedNodesTree& nodesManager)
{
	GMeshDesc = RMeshDesc{};
	ParseNode(nodesManager.Nodes, pScene->mRootNode, pScene->mMeshes);
}*/

void ParseGeometry(aiMesh** meshesArr, UINT numMeshes, RGeometryBufferCPU& geometry)
{
	auto& VBPosition = geometry.VertexBufferPositionsCPU;
	auto& VBRest = geometry.VertexBufferRestCPU;
	auto& IB = geometry.IndexBufferCPU.Indices;

	auto numVertices = GetNumVertices(meshesArr, numMeshes);
	VBPosition.TAllocate<float3>(numVertices);
	VBRest.TAllocate<VertexInputRest>(numVertices);

	for (int m = 0; m < numMeshes; m++)
	{
		const aiMesh* pMesh = meshesArr[m];

		assert(pMesh->HasNormals(), pMesh->HasPositions(), pMesh->HasTangentsAndBitangents(), pMesh->HasTextureCoords());

		//VB
		for (int v = 0; v < pMesh->mNumVertices; v++)
		{
			float3& position = VBPosition.TGetVertexRef<float3>(v);
			VertexInputRest& vertex = VBRest.TGetVertexRef<VertexInputRest>(v);

			std::memcpy(&position, &pMesh->mVertices[v], sizeof(float3));
			std::memcpy(&vertex.Normal, &pMesh->mNormals[v], sizeof(float3));
			std::memcpy(&vertex.Tangent, &pMesh->mTangents[v], sizeof(float3));
			std::memcpy(&vertex.Bitangent, &pMesh->mBitangents[v], sizeof(float3));
			std::memcpy(&vertex.TexCoord, &pMesh->mTextureCoords[0][v], sizeof(float2));
		}

		//IB
		for (int f = 0; f < pMesh->mNumFaces; f++)
		{
			const auto& face = pMesh->mFaces[f];
			assert(face.mNumIndices == 3);
			IB.push_back(face.mIndices[0]);
			IB.push_back(face.mIndices[1]);
			IB.push_back(face.mIndices[2]);
		}
	}
}

void RModelImporter::Import(const std::string& fileName, RGeometryBufferCPU& geometry)// , RDrawableInstancedNodesTree& nodesTree)
{
	PRINTOUT("Loading model" << fileName.c_str() << std::endl)
	AssimpImporter importer;

	const aiScene* pScene = importer.ReadModelAssimp(fileName);

	//ParseNodes(pScene, nodesTree);

	ParseGeometry(pScene->mMeshes, pScene->mNumMeshes, geometry);

	assert(geometry.VertexBufferPositionsCPU.Data.data());
	assert(!geometry.IndexBufferCPU.Indices.empty());

}
