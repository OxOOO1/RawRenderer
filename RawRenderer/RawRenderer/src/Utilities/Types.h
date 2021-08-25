#pragma once

#include <DirectXMath.h>
#include <Windows.h>

using namespace DirectX;
#define INLINE __forceinline

struct UINT2
{

	bool operator ==(UINT2 to)
	{
		return this->x == to.x && this->y == to.y;
	}

	bool operator <(UINT2 than)
	{
		return this->x < than.x && this->x < than.y;
	}

	unsigned int x = 0;
	unsigned int y = 0;
};

struct UINT3
{
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int z = 0;
};

struct INT3
{
	int x = 0;
	int y = 0;
	int z = 0;
};
typedef INT3 int3;

struct FLOAT2
{
	float x = 0;
	float y = 0;
};

struct FLOAT3
{
	float x = 0;
	float y = 0;
	float z = 0;

	inline operator XMFLOAT3() const { return XMFLOAT3{ x,y,z }; }

};

struct FLOAT4
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;

	FLOAT4()
	{

	}

	FLOAT4(const FLOAT3& f)
	{
		x = f.x;
		y = f.y;
		z = f.z;
		w = 1.f;
	}
	FLOAT4(const XMFLOAT4& f)
	{
		x = f.x;
		y = f.y;
		z = f.z;
		w = f.w;
	}
	FLOAT4(float xx, float yy, float zz, float ww)
	{
		x = xx;
		y = yy;
		z = zz;
		w = ww;
	}

	inline operator XMFLOAT4() const { return XMFLOAT4{ x,y,z,w }; }
};

typedef DirectX::XMMATRIX matrix;
typedef UINT2 uint2;
typedef UINT3 uint3;
typedef FLOAT2 float2;
typedef FLOAT3 float3;
typedef FLOAT4 float4;
typedef UINT uint;

class RFloat4;

class RFloat3
{
public:

	INLINE RFloat3() {}
	INLINE RFloat3(float x, float y, float z) { VectorXM = XMVectorSet(x, y, z, z); }
	INLINE RFloat3(const XMFLOAT3& v) { VectorXM = XMLoadFloat3(&v); }
	INLINE RFloat3(const RFloat3& v) { VectorXM = v.VectorXM; }
	INLINE RFloat3(const float3& v) { VectorXM = XMVectorSet(v.x, v.y, v.z, v.z); }
	INLINE RFloat3(const int3& v) { VectorXM = XMVectorSet(v.x, v.y, v.z, v.z); }
	INLINE RFloat3(const XMVECTOR& vec) { VectorXM = vec; }

	INLINE operator XMVECTOR() const { return VectorXM; }
	INLINE operator XMFLOAT3() const 
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, VectorXM);
		return ret;
	}
	INLINE operator float3() const
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, VectorXM);
		return float3{ ret.x, ret.y, ret.z };
	}
	INLINE operator int3() const
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, VectorXM);
		return int3{static_cast<int>(roundf(ret.x)), static_cast<int>(roundf(ret.y)), static_cast<int>(roundf(ret.z)) };
	}

	void Normalize()
	{
		VectorXM = DirectX::XMVector3Normalize(VectorXM);
	}

	inline RFloat3 Rotate(class RQuaternion rotation) const;

	RFloat3 Transform(const XMMATRIX& transform) const
	{
		return DirectX::XMVector3Transform(VectorXM, transform);
	}

	INLINE float GetLength()
	{
		float ret;
		DirectX::XMStoreFloat(&ret,DirectX::XMVector3Length(VectorXM));
		return ret;
	}
	INLINE float GetLengthSq()
	{
		float ret;
		DirectX::XMStoreFloat(&ret, DirectX::XMVector3LengthSq(VectorXM));
		return ret;
	}
	INLINE float GetLengthEst()
	{
		float ret;
		DirectX::XMStoreFloat(&ret, DirectX::XMVector3LengthEst(VectorXM));
		return ret;
	}

	INLINE RFloat3 operator- () const { return RFloat3(XMVectorNegate(VectorXM)); }
	INLINE RFloat3 operator+ (RFloat3 v2) const { return RFloat3(XMVectorAdd(VectorXM, v2)); }
	INLINE RFloat3 operator- (RFloat3 v2) const { return RFloat3(XMVectorSubtract(VectorXM, v2)); }
	INLINE RFloat3 operator* (RFloat3 v2) const { return RFloat3(XMVectorMultiply(VectorXM, v2)); }
	INLINE RFloat3 operator/ (RFloat3 v2) const { return RFloat3(XMVectorDivide(VectorXM, v2)); }
	INLINE RFloat3 operator* (int  v2) const { return RFloat3(XMVectorScale(VectorXM, v2)); }
	INLINE RFloat3 operator/ (int  v2) const { return RFloat3(VectorXM/v2); }
	INLINE RFloat3 operator* (float  v2) const { return RFloat3(XMVectorScale(VectorXM, v2)); }
	INLINE RFloat3 operator/ (float  v2) const { return RFloat3(VectorXM / v2); }

	INLINE RFloat3& operator += (RFloat3 v) { *this = *this + v; return *this; }
	INLINE RFloat3& operator -= (RFloat3 v) { *this = *this - v; return *this; }
	INLINE RFloat3& operator *= (RFloat3 v) { *this = *this * v; return *this; }
	INLINE RFloat3& operator /= (RFloat3 v) { *this = *this / v; return *this; }

	XMVECTOR VectorXM;

};


class RFloat4
{
public:

	INLINE RFloat4() {}
	INLINE RFloat4(float x, float y, float z, float w) { VectorXM = XMVectorSet(x, y, z, w); }
	INLINE RFloat4(const XMFLOAT4& v) { VectorXM = XMLoadFloat4(&v); }
	INLINE RFloat4(const RFloat4& v) { VectorXM = v.VectorXM; }
	INLINE RFloat4(const RFloat3& v) { VectorXM = v.VectorXM; XMVectorSetW(VectorXM, 1); }
	INLINE RFloat4(float4 v) { VectorXM = XMVectorSet(v.x, v.y, v.z, v.w); }
	INLINE explicit RFloat4(const XMVECTOR& vec) { VectorXM = vec; }

	INLINE operator XMVECTOR() const { return VectorXM; }
	INLINE operator XMFLOAT4() const
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, VectorXM);
		return ret;
	}
	INLINE operator float4() const
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, VectorXM);
		return float4{ ret };
	}

	RFloat4 Transform(const XMMATRIX& transform) const
	{
		return RFloat4(DirectX::XMVector4Transform(VectorXM, transform));
	}

	INLINE RFloat4 operator- () const { return RFloat4(XMVectorNegate(VectorXM)); }
	INLINE RFloat4 operator+ (RFloat4 v2) const { return RFloat4(XMVectorAdd(VectorXM, v2)); }
	INLINE RFloat4 operator- (RFloat4 v2) const { return RFloat4(XMVectorSubtract(VectorXM, v2)); }
	INLINE RFloat4 operator* (RFloat4 v2) const { return RFloat4(XMVectorMultiply(VectorXM, v2)); }
	INLINE RFloat4 operator/ (RFloat4 v2) const { return RFloat4(XMVectorDivide(VectorXM, v2)); }
	INLINE RFloat4 operator* (int  v2) const { return RFloat4(XMVectorScale(VectorXM, v2)); }
	INLINE RFloat4 operator/ (int  v2) const { return RFloat4(VectorXM / v2); }
	INLINE RFloat4 operator* (float  v2) const { return RFloat4(XMVectorScale(VectorXM, v2)); }
	INLINE RFloat4 operator/ (float  v2) const { return RFloat4(VectorXM / v2); }

	INLINE RFloat4& operator += (RFloat4 v) { *this = *this + v; return *this; }
	INLINE RFloat4& operator -= (RFloat4 v) { *this = *this - v; return *this; }
	INLINE RFloat4& operator *= (RFloat4 v) { *this = *this * v; return *this; }
	INLINE RFloat4& operator /= (RFloat4 v) { *this = *this / v; return *this; }

	XMVECTOR VectorXM;

};


class RQuaternion
{
public:
	INLINE RQuaternion() { QuaternionXM = XMQuaternionIdentity(); }
	INLINE RQuaternion(const RFloat3& axis, float angle) { QuaternionXM = XMQuaternionRotationAxis(axis, angle); }
	INLINE RQuaternion(float pitch, float yaw, float roll) { QuaternionXM = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll); }
	INLINE RQuaternion(float3 pitchYawRoll) { QuaternionXM = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z); }
	INLINE explicit RQuaternion(const XMMATRIX& matrix) { QuaternionXM = XMQuaternionRotationMatrix(matrix); }
	INLINE explicit RQuaternion(const XMVECTOR& vec) { QuaternionXM = vec; }
	INLINE RQuaternion(float4 quat) { QuaternionXM = XMVectorSet(quat.x, quat.y, quat.z, quat.w); }

	INLINE void Update(float3 pitchYawRoll) { QuaternionXM = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z); }

	INLINE operator XMVECTOR() const { return QuaternionXM; }
	INLINE operator XMFLOAT4() const { XMFLOAT4 ret; DirectX::XMStoreFloat4(&ret, QuaternionXM); return ret; }
	INLINE operator float4() const { XMFLOAT4 ret; DirectX::XMStoreFloat4(&ret, QuaternionXM); return float4(ret); }

	INLINE RQuaternion operator~ (void) const { return RQuaternion(XMQuaternionConjugate(QuaternionXM)); }
	INLINE RQuaternion operator- (void) const { return RQuaternion(XMVectorNegate(QuaternionXM)); }

	INLINE RQuaternion operator* (RQuaternion rhs) const { return RQuaternion(XMQuaternionMultiply(rhs, QuaternionXM)); }
	INLINE RFloat3 operator* (RFloat3 rhs) const { return RFloat3(XMVector3Rotate(rhs, QuaternionXM)); }
	RFloat3 RotateVector(RFloat3 vec) const { return RFloat3(XMVector3Rotate(vec, QuaternionXM)); }

	INLINE RQuaternion& operator= (RQuaternion rhs) { QuaternionXM = rhs; return *this; }
	INLINE RQuaternion& operator*= (RQuaternion rhs) { *this = *this * rhs; return *this; }

protected:
	XMVECTOR QuaternionXM;
};


RFloat3 RFloat3::Rotate(class RQuaternion rotation) const
{
	return DirectX::XMVector3Rotate(VectorXM, rotation);
}