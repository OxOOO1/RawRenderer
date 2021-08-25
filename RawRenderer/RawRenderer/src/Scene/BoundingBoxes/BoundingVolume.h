#pragma once

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>
#include <array>
#include "Utilities/Types.h"

class RBoundingBox
{
public:

	static constexpr UINT NumCorners = 8;

	void CreateFromVertices(UINT numVertices, const char* data, UINT stride);

	void SetPosition(const RFloat3& position)
	{
		Position = position;
	}

	void Translate(const RFloat3& offset)
	{
		Position = Position + offset;
	}
	
	void Scale(float factor)
	{
		Extents * factor;
	}

	void SetOrientation(RQuaternion orientation);

	void SetTransform(DirectX::XMMATRIX mat);

	void GetCorners(std::array<float3, NumCorners> cornersArr);

	const RFloat3& GetExtents() const
	{
		return Extents;
	}

	const RFloat3& GetPosition() const 
	{
		return Position;
	}

protected:
	RFloat3 Position{ 0,0,0 };
	RFloat3 Extents{ 1,1,1 };

	RFloat3 OriginalPosition{ 0,0,0 };
	RFloat3 OriginalExtents{ 1,1,1 };

	const RFloat3 AxisAlignedNormalizedBoxCorners[NumCorners] =
	{
		{ -1.0f, -1.0f,  1.0f },
		{  1.0f, -1.0f,  1.0f },
		{  1.0f,  1.0f,  1.0f },
		{ -1.0f,  1.0f,  1.0f },
		{ -1.0f, -1.0f, -1.0f },
		{  1.0f, -1.0f, -1.0f },
		{  1.0f,  1.0f, -1.0f },
		{ -1.0f,  1.0f, -1.0f },
	};

};

class RBoundingBoxOriented : public RBoundingBox
{
public:

	void Rotate(RQuaternion rotation);

	void CreateFromVertices(UINT numVertices, float3* data, UINT stride);

	void GetCorners(std::array<float3, NumCorners>& corners);

private:
	RQuaternion Orientation;

	RQuaternion OriginalOrientation;

};



class RBoundingSphere
{
public:

	void CreateFromVertices(UINT numVertices, float3* data, UINT stride);

	RFloat3 Position;
	float Radius;

	RFloat3 OriginalPosition;
	float OriginalRadius;
};


class RBoundingFrustum
{
public:

	static constexpr UINT NumCorners = 8;

	void CreateFromMatrix(const XMMATRIX& projection);

	void GetCorners(std::array<float3, NumCorners>& Corners);

	void Transform(RQuaternion rotation, RFloat3 translation);

	RFloat3 Center;
	RFloat3 Origin;
	RQuaternion Orientation;

	float RightSlope;           // Positive X (X/Z)
	float LeftSlope;            // Negative X
	float TopSlope;             // Positive Y (Y/Z)
	float BottomSlope;          // Negative Y
	float Near, Far;            // Z of the near plane and far plane.

	std::array<float3, NumCorners> VertexBuffer;

};

class RBoundingPlane
{
public:

	RBoundingPlane(RFloat3 point, RFloat3 normal)
	{
		Plane = RFloat4(DirectX::XMPlaneFromPointNormal(point, normal));
	}

	RFloat4 Plane;
};

class RBoundingVolumesManagerS
{
public:

	static UINT CreateNewBox(class RGeometryBufferCPU& mesh);

	static UINT CreateNewBox(const RBoundingBox& box)
	{
		UINT index = BoundingBoxes.size();
		BoundingBoxes.push_back(box);
		return index;
	}

	static RBoundingBox& GetBox(UINT index)
	{
		return BoundingBoxes.at(index);
	}

	static void SetPosition(uint index, const float3& position)
	{
		BoundingBoxes.at(index).SetPosition(position);
	}

	static std::vector<RBoundingBox> BoundingBoxes;
	static std::vector<RBoundingSphere> BoundingSpheres;
};

