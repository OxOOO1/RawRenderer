#pragma once

#include "Drawable.h"


class RDebugDrawable
{
public:

	enum EType
	{
		PLANE = 6,
		CUBE = 36,
		CONE = 192,
		CIRCLE = 96,
		ICOSPHERE = 240,
		UVSPHERE = 2880,
	};

	struct InstanceDescription
	{
		matrix TransformMat;
		float4 Color;
	};

	RDebugDrawable(EType inType)
	{
		Type = inType;

		InstancesBuffer.Create(L"DebugDrawMatricesBuffer", 1000, sizeof(InstanceDescription));
	}

	EType Type;

	void UploadInstance(InstanceDescription* instanceDesc, UINT instanceIndex);

	UINT NumInstances = 0;

	RStructuredBuffer InstancesBuffer;

};

struct RDebugInstanceRef
{
	RDebugDrawable::EType Type;
	UINT Index;
};

class RDebugDrawManagerS
{
public:

	static RDebugDrawManagerS& Get()
	{
		static RDebugDrawManagerS debugDraw;
		return debugDraw;
	}

	static UINT UploadInstance(const RDebugInstanceRef& instanceRef, RDebugDrawable::InstanceDescription& desc)
	{
		RDebugDrawable* drawable = GetDrawableBasedOnType(instanceRef.Type);
		drawable->UploadInstance(&desc, instanceRef.Index);
	}

	static UINT CreateInstance(RDebugDrawable::EType DrawableType)
	{
		auto& G = Get();
		RDebugDrawable* drawable = GetDrawableBasedOnType(DrawableType);
		return drawable->NumInstances++;
	}

private:

	static RDebugDrawable* GetDrawableBasedOnType(RDebugDrawable::EType type)
	{
		auto& G = Get();
		switch (type)
		{
		case RDebugDrawable::PLANE:
			return &G.Plane;
			break;
		case RDebugDrawable::CUBE:
			return &G.Cube;
			break;
		case RDebugDrawable::CONE:
			return &G.Cone;
			break;
		case RDebugDrawable::CIRCLE:
			return &G.Circle;
			break;
		case RDebugDrawable::ICOSPHERE:
			return &G.Icosphere;
			break;
		case RDebugDrawable::UVSPHERE:
			return &G.Sphere;
			break;
		default:
			return nullptr;
			break;
		}
	}



	RDebugDrawable Plane{ RDebugDrawable::PLANE };
	RDebugDrawable Cube{ RDebugDrawable::CUBE };
	RDebugDrawable Circle{ RDebugDrawable::CIRCLE };
	RDebugDrawable Cone{ RDebugDrawable::CONE };
	RDebugDrawable Sphere{ RDebugDrawable::UVSPHERE };
	RDebugDrawable Icosphere{ RDebugDrawable::ICOSPHERE };

};

