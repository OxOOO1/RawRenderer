#pragma once

#include "Utilities/Types.h"
#include "Scene\DrawableComponent\Node.h"

using namespace DirectX;

class RTransformSimple
{
public:

	void SetPosition(const RFloat3& position)
	{
		bWasUpdated = true;
		Position = position;
	}

	void SetOrientation(const RQuaternion& orientation)
	{
		bWasUpdated = true;
		Orientation = orientation;
	}

	void SetUniformScale(float scale)
	{
		bWasUpdated = true;
		Scale = { scale,scale,scale };
	}

	void RotateY90()
	{
		bWasUpdated = true;
		Rotation.y = DirectX::XMScalarModAngle(Rotation.y += DirectX::XMConvertToRadians(90));
		UpdateOrientationFromEuler();
	}
	void RotateX90()
	{
		bWasUpdated = true;
		Rotation.y = DirectX::XMScalarModAngle(Rotation.x += DirectX::XMConvertToRadians(90));
		UpdateOrientationFromEuler();
	}

	DirectX::XMMATRIX GetTransformMatrix()
	{
		XMMATRIX outM;
		outM = DirectX::XMMatrixIdentity();
		outM = DirectX::XMMatrixMultiply(outM, DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z));
		outM = DirectX::XMMatrixMultiply(outM, DirectX::XMMatrixRotationQuaternion(Orientation));
		outM = DirectX::XMMatrixMultiply(outM, DirectX::XMMatrixTranslationFromVector(Position));
		return outM;
	}

	void UpdateOrientationFromEuler()
	{
		Orientation.Update(Rotation);
	}

	RFloat3 Position{ 0,0,0 };
	float3 Rotation{ 0,0,0 };
	RQuaternion Orientation;
	float3 Scale{ 1,1,1 };

	bool bWasUpdated = true;
};

class RTransform : public RTransformSimple
{
public:

	void UpdateTransformLocal()
	{
		auto mat = DirectX::XMMatrixIdentity();
		mat = DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z));
		mat = DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixRotationQuaternion(Orientation));
		MatLocalSpace = DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixTranslationFromVector(Position));
	}

	XMMATRIX MatLocalSpace{ DirectX::XMMatrixIdentity() };
	XMMATRIX MatParentSpace{ DirectX::XMMatrixIdentity() };
	XMMATRIX MatWorldSpace{ DirectX::XMMatrixIdentity() };
	XMMATRIX MatDefaultWorldSpaceInv{ DirectX::XMMatrixIdentity() };

};
/*

class RTransformNode : public RNode
{
public:

	RTransform Transform;

};

class RTransformTree
{
public:

	std::vector<RTransformNode> Nodes;

};*/


