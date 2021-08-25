#pragma once

#include <vector>
#include <DirectXMath.h>
#include <Windows.h>
#include <d3d12.h>

class RVertexAttribute
{
public:
	struct BGRAColor
	{
		unsigned char r, g, b;
		unsigned char a = 255;
	};

	struct Weights
	{
		UINT8 weight[4];
	};

	struct BoneIndices
	{
		UINT8 index[4];
	};

	enum EAttribType
	{
		Position2D,
		Position3D,
		Position3DPlusIndex,
		TexCoord,
		Normal,
		Tangent,
		Bitangent,
		Float3Color,
		Float4Color,
		BGRAColor,
		BoneIndices,
		Weights
	};

	template<EAttribType AttribType>
	struct RVertexAttributeDesc;

	template<>
	struct RVertexAttributeDesc<Position3D>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		static constexpr const char* semantic = "Position";
	};

	template<>
	struct RVertexAttributeDesc<TexCoord>
	{
		using SysType = DirectX::XMFLOAT2;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
		static constexpr const char* semantic = "TexCoord";
	};

	template<>
	struct RVertexAttributeDesc<Normal>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Normal";
	};

	template<>
	struct RVertexAttributeDesc<Tangent>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Tangent";
	};

	template<>
	struct RVertexAttributeDesc<Bitangent>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Bitangent";
	};


#define GEN_CASE(x)\
	case x:\
	return sizeof(RVertexAttributeDesc<x>::SysType);\
	break;

	static constexpr UINT SizeOf(EAttribType Type)
	{
		switch (Type)
		{
		GEN_CASE(Position3D)
		GEN_CASE(TexCoord)
		GEN_CASE(Normal)
		GEN_CASE(Tangent)
		GEN_CASE(Bitangent)
		}
		assert("Invalid type" && false);
		return 0;
	}


public:
	RVertexAttribute() = default;
	RVertexAttribute(EAttribType AttribType, UINT offset, UINT inputSlot = 0)
		: Type(AttribType), Offset(offset), InputSlot(inputSlot)
	{}

	UINT GetSize()
	{
		return SizeOf(Type);
	}

	UINT GetOffset()
	{
		return Offset;
	}

	UINT GetOffsetAfter()
	{
		return Offset + GetSize();
	}

	EAttribType GetType()
	{
		return Type;
	}

#define GEN_CASE(x)\
	case RVertexAttribute::x:\
	return TGenerateDesc<x>(Offset, InputSlot);\
	break;
	D3D12_INPUT_ELEMENT_DESC GenerateDesc()
	{
		switch (Type)
		{
		GEN_CASE(Position3D)
		GEN_CASE(TexCoord)
		GEN_CASE(Normal)
		GEN_CASE(Tangent)
		GEN_CASE(Bitangent)
		default:
			break;
		}
	}

	template <EAttribType Type>
	static constexpr D3D12_INPUT_ELEMENT_DESC TGenerateDesc(UINT Offset, UINT8 inputSlot)
	{
		return { RVertexAttributeDesc<Type>::semantic, 0, RVertexAttributeDesc<Type>::dxgiFormat, inputSlot, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	}

private:
	EAttribType Type;
	UINT Offset;
	UINT8 InputSlot;
};


struct RVertexDesc
{
	UINT8 GetSizeBytes()
	{
		return Attributes.empty() ? 0u : Attributes.back().GetOffsetAfter();
	}

	std::vector<RVertexAttribute> Attributes;
};

class RVertexRef
{
public:

	RVertexRef(char* cpuAddress, UINT8 vertexSize)
		:CpuAddress(cpuAddress)
		//, Desc(desc)
		,VertexSize(vertexSize)
	{
	}

	template<RVertexAttribute::EAttribType Type> 
	void EmplaceBackAttribute(void* data)
	{
		using CurType = typename RVertexAttribute::RVertexAttributeDesc<Type>::SysType;
		//*reinterpret_cast<CurType*>(CpuAddress) = *reinterpret_cast<CurType*>(data);
		std::memcpy(CpuAddress, data, sizeof(CurType));
		CpuAddress += RVertexAttribute::SizeOf(Type);
		assert(CurOffset <= VertexSize);
		CurOffset += RVertexAttribute::SizeOf(Type);
	}

	template <typename T>
	void Copy(T* data)
	{
		std::memcpy(CpuAddress, data, sizeof(T));
	}



private:
	char* CpuAddress;
	UINT8 CurOffset = 0;
	UINT8 VertexSize;
	//const RVertexDesc& Desc;
};

struct RVertexBufferCPU
{
	UINT GetNumVertices()
	{
		assert(!Data.empty());
		//return Data.size() / VertexDesc.GetSizeBytes();
		return NumVertices;
	}

	UINT GetVertexSize()
	{
		assert(!Data.empty());
		return VertexSize;
	}

	UINT GetSizeBytes()
	{
		return Data.size();
	}

	template<typename T>
	void TAllocate(UINT numVertices)
	{
		assert(Data.empty());
		Data.resize(sizeof(T) * numVertices);
		NumVertices = numVertices;
		VertexSize = sizeof(T);
	}

	template <typename V>
	V& TGetVertexRef(UINT index)
	{
		assert(index < NumVertices);
		return *reinterpret_cast<V*>(Data.data() + sizeof(V) * index);
	}

	char* GetVertexPointer(UINT index)
	{
		assert(index < NumVertices);
		return Data.data() + GetVertexSize() * index;
	}

	std::vector<char> Data;
	UINT NumVertices = 0;
	UINT VertexSize = 0;
	//RVertexDesc VertexDesc;


	/*RVertexRef Allocate()
	{
		Data.resize(Data.size() + VertexDesc.GetSizeBytes());
		return RVertexRef{ Data.data() + Data.size() - VertexDesc.GetSizeBytes(), VertexDesc.GetSizeBytes() };
	}

	void Allocate(UINT NumVertices)
	{
		Data.resize(Data.size() + VertexDesc.GetSizeBytes() * NumVertices);
	}

	RVertexRef GetVertexRef(UINT index)
	{
		assert(index < GetNumVertices());
		return RVertexRef{ Data.data() + VertexDesc.GetSizeBytes() * index, VertexDesc.GetSizeBytes() };
	}*/


	/*void Init(const RVertexAttribute::EAttribType* attributes, UINT numAttributes)
	{
		UINT offset = 0;
		for (int i = 0; i < numAttributes; i++)
		{
			VertexDesc.Attributes.push_back(RVertexAttribute{ attributes[i], offset });
			offset += RVertexAttribute::SizeOf(attributes[i]);
		}

	}*/


};