#include "BoundingVolume.h"

#include "DrawableComponent/Drawable.h"

void RBoundingBox::CreateFromVertices(UINT numVertices, const char* data, UINT stride)
{
	assert(numVertices > 0);
	assert(data);

	// Find the minimum and maximum x, y, and z
	RFloat3 vMin, vMax;

	vMin = vMax = *reinterpret_cast<const float3*>(data);

	for (size_t i = 1; i < numVertices; ++i)
	{
		RFloat3 pos = *reinterpret_cast<const float3*>(data + i * stride);

		vMin = XMVectorMin(vMin, pos);
		vMax = XMVectorMax(vMax, pos);
	}

	// Store center and extents.
	OriginalPosition = XMVectorScale(XMVectorAdd(vMin, vMax), 0.5f);
	OriginalExtents = XMVectorScale(XMVectorSubtract(vMax, vMin), 0.5f);

	Position = OriginalPosition;
	Extents = OriginalExtents;
}

void RBoundingBox::SetOrientation(RQuaternion rotation)
{
	// Compute and transform the corners and find new min/max bounds.
	RFloat3 Corner = XMVectorMultiplyAdd(OriginalExtents, AxisAlignedNormalizedBoxCorners[0], OriginalPosition);
	Corner = XMVector3Rotate(Corner, rotation);

	RFloat3 Min, Max;
	Min = Max = Corner;

	for (size_t i = 1; i < NumCorners; ++i)
	{
		Corner = XMVectorMultiplyAdd(OriginalExtents, AxisAlignedNormalizedBoxCorners[i], OriginalPosition);
		Corner = XMVector3Rotate(Corner, rotation);

		Min = XMVectorMin(Min, Corner);
		Max = XMVectorMax(Max, Corner);
	}

	// Store center and extents.
	Position = XMVectorScale(XMVectorAdd(Min, Max), 0.5f);
	Extents = XMVectorScale(XMVectorSubtract(Max, Min), 0.5f);
}

void RBoundingBox::SetTransform(DirectX::XMMATRIX mat)
{
	// Compute and transform the corners and find new min/max bounds.
	RFloat3 Corner = XMVectorMultiplyAdd(OriginalExtents, AxisAlignedNormalizedBoxCorners[0], OriginalPosition);
	Corner = XMVector3Transform(Corner, mat);

	RFloat3 Min, Max;
	Min = Max = Corner;

	for (size_t i = 1; i < NumCorners; ++i)
	{
		Corner = XMVectorMultiplyAdd(OriginalExtents, AxisAlignedNormalizedBoxCorners[i], OriginalPosition);
		Corner = XMVector3Transform(Corner, mat);

		Min = XMVectorMin(Min, Corner);
		Max = XMVectorMax(Max, Corner);
	}

	// Store center and extents.
	Position = XMVectorScale(XMVectorAdd(Min, Max), 0.5f);
	Extents = XMVectorScale(XMVectorSubtract(Max, Min), 0.5f);
}

void RBoundingBox::GetCorners(std::array<float3, NumCorners> cornersArr)
{
	for (size_t i = 0; i < NumCorners; ++i)
	{
		RFloat3 c = XMVectorMultiplyAdd(Extents, AxisAlignedNormalizedBoxCorners[i], Position);
		cornersArr[i] = c;
	}
}

void RBoundingBoxOriented::Rotate(RQuaternion rotation)
{
	assert(DirectX::Internal::XMQuaternionIsUnit(rotation));
	assert(DirectX::Internal::XMQuaternionIsUnit(Orientation));
	// Composite the box rotation and the transform rotation.
	Orientation = RQuaternion(XMQuaternionMultiply(Orientation, rotation));
}

void RBoundingBoxOriented::CreateFromVertices(UINT numVertices, float3* data, UINT stride)
{
	assert(numVertices > 0);
	assert(data != nullptr);

	XMVECTOR CenterOfMass = XMVectorZero();

	// Compute the center of mass and inertia tensor of the points.
	for (size_t i = 0; i < numVertices; ++i)
	{
		XMVECTOR Point = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(reinterpret_cast<const uint8_t*>(data) + i * stride));

		CenterOfMass = XMVectorAdd(CenterOfMass, Point);
	}

	//CenterOfMass divide by NumOfVertices
	CenterOfMass = XMVectorMultiply(CenterOfMass, XMVectorReciprocal(XMVectorReplicate(float(numVertices))));

	// Compute the inertia tensor of the points around the center of mass.
	// Using the center of mass is not strictly necessary, but will hopefully
	// improve the stability of finding the eigenvectors.
	XMVECTOR XX_YY_ZZ = XMVectorZero();
	XMVECTOR XY_XZ_YZ = XMVectorZero();

	for (size_t i = 0; i < numVertices; ++i)
	{
		XMVECTOR Point = XMVectorSubtract(XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(reinterpret_cast<const uint8_t*>(data) + i * stride)), CenterOfMass);

		XX_YY_ZZ = XMVectorAdd(XX_YY_ZZ, XMVectorMultiply(Point, Point));

		XMVECTOR XXY = XMVectorSwizzle<XM_SWIZZLE_X, XM_SWIZZLE_X, XM_SWIZZLE_Y, XM_SWIZZLE_W>(Point);
		XMVECTOR YZZ = XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_Z, XM_SWIZZLE_Z, XM_SWIZZLE_W>(Point);

		XY_XZ_YZ = XMVectorAdd(XY_XZ_YZ, XMVectorMultiply(XXY, YZZ));
	}

	XMVECTOR v1, v2, v3;

	// Compute the eigenvectors of the inertia tensor.
	DirectX::Internal::CalculateEigenVectorsFromCovarianceMatrix(XMVectorGetX(XX_YY_ZZ), XMVectorGetY(XX_YY_ZZ),
		XMVectorGetZ(XX_YY_ZZ),
		XMVectorGetX(XY_XZ_YZ), XMVectorGetY(XY_XZ_YZ),
		XMVectorGetZ(XY_XZ_YZ),
		&v1, &v2, &v3);

	// Put them in a matrix.
	XMMATRIX R;

	R.r[0] = XMVectorSetW(v1, 0.f);
	R.r[1] = XMVectorSetW(v2, 0.f);
	R.r[2] = XMVectorSetW(v3, 0.f);
	R.r[3] = g_XMIdentityR3.v;

	// Multiply by -1 to convert the matrix into a right handed coordinate
	// system (Det ~= 1) in case the eigenvectors form a left handed
	// coordinate system (Det ~= -1) because XMQuaternionRotationMatrix only
	// works on right handed matrices.
	XMVECTOR Det = XMMatrixDeterminant(R);

	if (XMVector4Less(Det, XMVectorZero()))
	{
		R.r[0] = XMVectorMultiply(R.r[0], g_XMNegativeOne.v);
		R.r[1] = XMVectorMultiply(R.r[1], g_XMNegativeOne.v);
		R.r[2] = XMVectorMultiply(R.r[2], g_XMNegativeOne.v);
	}

	// Get the rotation quaternion from the matrix.
	XMVECTOR vOrientation = XMQuaternionRotationMatrix(R);

	// Make sure it is normal (in case the vectors are slightly non-orthogonal).
	vOrientation = XMQuaternionNormalize(vOrientation);

	// Rebuild the rotation matrix from the quaternion.
	R = XMMatrixRotationQuaternion(vOrientation);

	// Build the rotation into the rotated space.
	XMMATRIX InverseR = XMMatrixTranspose(R);

	// Find the minimum OBB using the eigenvectors as the axes.
	XMVECTOR vMin, vMax;

	vMin = vMax = XMVector3TransformNormal(XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(data)), InverseR);

	for (size_t i = 1; i < numVertices; ++i)
	{
		XMVECTOR Point = XMVector3TransformNormal(XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(reinterpret_cast<const uint8_t*>(data) + i * stride)),
			InverseR);

		vMin = XMVectorMin(vMin, Point);
		vMax = XMVectorMax(vMax, Point);
	}

	// Rotate the center into world space.
	XMVECTOR vCenter = XMVectorScale(XMVectorAdd(vMin, vMax), 0.5f);
	vCenter = XMVector3TransformNormal(vCenter, R);

	// Store center, extents, and orientation.
	Position = vCenter;
	Extents = XMVectorScale(XMVectorSubtract(vMax, vMin), 0.5f);
	Orientation = RQuaternion(vOrientation);
}

void RBoundingBoxOriented::GetCorners(std::array<float3, NumCorners>& corners)
{
	assert(DirectX::Internal::XMQuaternionIsUnit(Orientation));

	for (size_t i = 0; i < NumCorners; ++i)
	{
		RFloat3 C = XMVectorAdd(XMVector3Rotate(XMVectorMultiply(Extents, AxisAlignedNormalizedBoxCorners[i]), Orientation), Position);
		corners[i] = C;
	}
}

void RBoundingSphere::CreateFromVertices(UINT numVertices, float3* data, UINT stride)
{
	assert(numVertices > 0);
	assert(data);

	// Find the points with minimum and maximum x, y, and z
	XMVECTOR MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

	MinX = MaxX = MinY = MaxY = MinZ = MaxZ = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(data));

	for (size_t i = 1; i < numVertices; ++i)
	{
		XMVECTOR Point = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(reinterpret_cast<const uint8_t*>(data) + i * stride));

		float px = XMVectorGetX(Point);
		float py = XMVectorGetY(Point);
		float pz = XMVectorGetZ(Point);

		if (px < XMVectorGetX(MinX))
			MinX = Point;

		if (px > XMVectorGetX(MaxX))
			MaxX = Point;

		if (py < XMVectorGetY(MinY))
			MinY = Point;

		if (py > XMVectorGetY(MaxY))
			MaxY = Point;

		if (pz < XMVectorGetZ(MinZ))
			MinZ = Point;

		if (pz > XMVectorGetZ(MaxZ))
			MaxZ = Point;
	}

	// Use the min/max pair that are farthest apart to form the initial sphere.
	XMVECTOR DeltaX = XMVectorSubtract(MaxX, MinX);
	XMVECTOR DistX = XMVector3Length(DeltaX);

	XMVECTOR DeltaY = XMVectorSubtract(MaxY, MinY);
	XMVECTOR DistY = XMVector3Length(DeltaY);

	XMVECTOR DeltaZ = XMVectorSubtract(MaxZ, MinZ);
	XMVECTOR DistZ = XMVector3Length(DeltaZ);

	XMVECTOR vCenter;
	XMVECTOR vRadius;

	if (XMVector3Greater(DistX, DistY))
	{
		if (XMVector3Greater(DistX, DistZ))
		{
			// Use min/max x.
			vCenter = XMVectorLerp(MaxX, MinX, 0.5f);
			vRadius = XMVectorScale(DistX, 0.5f);
		}
		else
		{
			// Use min/max z.
			vCenter = XMVectorLerp(MaxZ, MinZ, 0.5f);
			vRadius = XMVectorScale(DistZ, 0.5f);
		}
	}
	else // Y >= X
	{
		if (XMVector3Greater(DistY, DistZ))
		{
			// Use min/max y.
			vCenter = XMVectorLerp(MaxY, MinY, 0.5f);
			vRadius = XMVectorScale(DistY, 0.5f);
		}
		else
		{
			// Use min/max z.
			vCenter = XMVectorLerp(MaxZ, MinZ, 0.5f);
			vRadius = XMVectorScale(DistZ, 0.5f);
		}
	}

	// Add any points not inside the sphere.
	for (size_t i = 0; i < numVertices; ++i)
	{
		XMVECTOR Point = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(reinterpret_cast<const uint8_t*>(data) + i * stride));

		XMVECTOR Delta = XMVectorSubtract(Point, vCenter);

		XMVECTOR Dist = XMVector3Length(Delta);

		if (XMVector3Greater(Dist, vRadius))
		{
			// Adjust sphere to include the new point.
			vRadius = XMVectorScale(XMVectorAdd(vRadius, Dist), 0.5f);
			vCenter = XMVectorAdd(vCenter, XMVectorMultiply(XMVectorSubtract(XMVectorReplicate(1.0f), XMVectorDivide(vRadius, Dist)), Delta));
		}
	}

	Position = vCenter;
	XMStoreFloat(&Radius, vRadius);
}

void RBoundingFrustum::CreateFromMatrix(const XMMATRIX& projection)
{
	//Cross-like Corners of the projection frustum in homogenous space.
	static XMVECTORF32 HomogenousPoints[6] =
	{
		{ { {  1.0f,  0.0f, 1.0f, 1.0f } } },   // right (at far plane)
		{ { { -1.0f,  0.0f, 1.0f, 1.0f } } },   // left
		{ { {  0.0f,  1.0f, 1.0f, 1.0f } } },   // top
		{ { {  0.0f, -1.0f, 1.0f, 1.0f } } },   // bottom

		{ { { 0.0f, 0.0f, 0.0f, 1.0f } } },     // near
		{ { { 0.0f, 0.0f, 1.0f, 1.0f } } }      // far
	};

	XMVECTOR Determinant;
	XMMATRIX matInverse = XMMatrixInverse(&Determinant, projection);

	Origin = RFloat3(0, 0, 0);

	// Convert from NDC to world(both world and camera since Origin 0)
	XMVECTOR Points[6];

	for (size_t i = 0; i < 6; ++i)
	{
		// Transform point.
		Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
	}

	// Compute the slopes.
	Points[0] = XMVectorMultiply(Points[0], XMVectorReciprocal(XMVectorSplatZ(Points[0])));
	Points[1] = XMVectorMultiply(Points[1], XMVectorReciprocal(XMVectorSplatZ(Points[1])));
	Points[2] = XMVectorMultiply(Points[2], XMVectorReciprocal(XMVectorSplatZ(Points[2])));
	Points[3] = XMVectorMultiply(Points[3], XMVectorReciprocal(XMVectorSplatZ(Points[3])));

	RightSlope = XMVectorGetX(Points[0]);
	LeftSlope = XMVectorGetX(Points[1]);
	TopSlope = XMVectorGetY(Points[2]);
	BottomSlope = XMVectorGetY(Points[3]);

	// Compute near and far.
	Points[4] = XMVectorMultiply(Points[4], XMVectorReciprocal(XMVectorSplatW(Points[4])));
	Points[5] = XMVectorMultiply(Points[5], XMVectorReciprocal(XMVectorSplatW(Points[5])));

	Near = XMVectorGetZ(Points[4]);
	Far = XMVectorGetZ(Points[5]);

	Center = Origin + RFloat3({ 0, 0, (Far - Near) / 2 });
}

void RBoundingFrustum::GetCorners(std::array<float3, NumCorners>& Corners)
{
	assert(DirectX::Internal::XMQuaternionIsUnit(Orientation));

	// Build the corners of the frustum.
	XMVECTOR vRightTop = XMVectorSet(RightSlope, TopSlope, 1.0f, 0.0f);
	XMVECTOR vRightBottom = XMVectorSet(RightSlope, BottomSlope, 1.0f, 0.0f);
	XMVECTOR vLeftTop = XMVectorSet(LeftSlope, TopSlope, 1.0f, 0.0f);
	XMVECTOR vLeftBottom = XMVectorSet(LeftSlope, BottomSlope, 1.0f, 0.0f);
	XMVECTOR vNear = XMVectorReplicatePtr(&Near);
	XMVECTOR vFar = XMVectorReplicatePtr(&Far);

	// Returns 8 corners position of bounding frustum.
	//     Near    Far
	//    0----1  4----5
	//    |    |  |    |
	//    |    |  |    |
	//    3----2  7----6

	XMVECTOR vCorners[NumCorners];
	vCorners[0] = XMVectorMultiply(vLeftTop, vNear);
	vCorners[1] = XMVectorMultiply(vRightTop, vNear);
	vCorners[2] = XMVectorMultiply(vRightBottom, vNear);
	vCorners[3] = XMVectorMultiply(vLeftBottom, vNear);
	vCorners[4] = XMVectorMultiply(vLeftTop, vFar);
	vCorners[5] = XMVectorMultiply(vRightTop, vFar);
	vCorners[6] = XMVectorMultiply(vRightBottom, vFar);
	vCorners[7] = XMVectorMultiply(vLeftBottom, vFar);

	for (size_t i = 0; i < NumCorners; ++i)
	{
		RFloat3 C = XMVectorAdd(XMVector3Rotate(vCorners[i], Orientation), Origin);
		Corners[i] = C;
	}
}

void RBoundingFrustum::Transform(RQuaternion rotation, RFloat3 translation)
{
	assert(DirectX::Internal::XMQuaternionIsUnit(rotation));

	assert(DirectX::Internal::XMQuaternionIsUnit(Orientation));

	// Composite the frustum rotation and the transform rotation.
	Orientation = RQuaternion(XMQuaternionMultiply(Orientation, rotation));

	// Transform the origin.
	Origin = XMVectorAdd(XMVector3Rotate(Origin, rotation), translation);
}

UINT RBoundingVolumesManagerS::CreateNewBox(RGeometryBufferCPU& mesh)
{
	UINT index = BoundingBoxes.size();
	RBoundingBox BoundingBox;
	BoundingBox.CreateFromVertices(mesh.VertexBufferPositionsCPU.GetNumVertices(), mesh.VertexBufferPositionsCPU.Data.data(), mesh.VertexBufferPositionsCPU.GetVertexSize());
	BoundingBoxes.push_back(std::move(BoundingBox));
	return index;
}

std::vector<RBoundingBox> RBoundingVolumesManagerS::BoundingBoxes;

std::vector<RBoundingSphere> RBoundingVolumesManagerS::BoundingSpheres;
