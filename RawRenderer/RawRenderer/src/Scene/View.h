#pragma once

#include <DirectXMath.h>

#include "Utilities/Types.h"

class RSceneView
{
public:

	RSceneView()
	{
		UpdateProjectionMatrix();
		UpdateViewMatrix();
	}

	RSceneView(bool isPerspective, UINT2 inDimension, float inNear, float inFar)
		: Dimension(inDimension), NearPlane(inNear), FarPlane(inFar), bPerspective(isPerspective)
	{
		UpdateProjectionMatrix();
		UpdateViewMatrix();
	}

	const RFloat3& GetPosition()
	{
		return Position;
	}

	void SetPosition(const RFloat3& position)
	{
		Position = position;
	}

	void GoTo(const RFloat3& position, float offset)
	{
		Position = position - (Direction * offset);
		UpdateViewMatrix();
	}

	const RFloat3& GetDirection()
	{
		return Direction;
	}
	const RFloat3& GetPosition() const
	{
		return Position;
	}

protected:
	void UpdateViewMatrix()
	{
		RightVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Direction, DirectX::g_XMIdentityR1));
		auto UpVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(RightVec, Direction));

		if (!bOrientationLocked)
		{
			Matrices.View = DirectX::XMMatrixLookAtLH(Position, DirectX::XMVectorAdd(Position, Direction), UpVec);
			Matrices.ViewProjection = DirectX::XMMatrixMultiply(Matrices.View, Matrices.Projection);
		}

	}
	void UpdateProjectionMatrix()
	{
		float Ratio = (float)Dimension.x / (float)Dimension.y;
		if (bPerspective)
			Matrices.Projection = DirectX::XMMatrixPerspectiveFovLH(FieldOfView, Ratio, NearPlane, FarPlane);
		else
			Matrices.Projection = DirectX::XMMatrixOrthographicLH((float)Dimension.x, (float)Dimension.y, NearPlane, FarPlane);
	}

public:
	struct ViewMatrices
	{
		DirectX::XMMATRIX View;
		DirectX::XMMATRIX Projection;
		DirectX::XMMATRIX ViewProjection;
	} Matrices;

protected:
	UINT2 Dimension{ 2000,2000};
	float NearPlane = 0.1f;
	float FarPlane = 1000.f;
	float FieldOfView{ DirectX::XMConvertToRadians(70.f) };
	bool bPerspective = true;

	RFloat3 Position{ 0,0,0 };
	RFloat3 Direction{0, 0, 1};
	RFloat3 RightVec;
	//RFloat3 UpVec;

	RQuaternion Orientation;
	bool bOrientationLocked = false;

	

	
};