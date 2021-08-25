#pragma once

#include "Events/EventObject.h"
#include "Input/Mouse.h"
#include "Input/Keyboard.h"
#include "Utilities/Time.h"

#include "View.h"
#include "ImGuiObject.h"

#include "Utilities/FileIO.h"

#include "thirdParty/ImGui/imgui.h"

class RCamera : public RSceneView, public RLevelEditorObject
{
public:
	RCamera(UINT2 Dimensions, float Near = 0.001f, float Far = 500.f)
		:RSceneView(true, Dimensions, Near, Far),
		RLevelEditorObject("MainCamera")
	{
		LoadFromFile();
		UpdateDirection();
		
	}

	~RCamera()
	{
		SaveToFile();
	}

	void OnUpdate()
	{
		if (!bLocked)
		{
			ProcessInput();
			auto offset = Mouse::Get().GetRawOffset();
			ProcessMouseMovement(offset.x, offset.y);
			UpdateViewMatrix();
		}

	}

	void OnEvent(Event& event)
	{
		if (event.CheckTypeAndHandle(EventType::MouseCursorEvent))
		{
			OnMouseCursorEvent(reinterpret_cast<MouseCursorEvent&>(event));
		}
	}

	void OnMouseCursorEvent(MouseCursorEvent& event)
	{
		if (event.state == MouseCursorEvent::CursorState::Hidden)
			bLocked = false;
		else if (event.state == MouseCursorEvent::CursorState::Shown)
			bLocked = true;
	}

	virtual void RenderUI() override;

	float2 GetNearFarPlanes() const
	{
		return { NearPlane, FarPlane };
	}

private:
	void UpdateDirection()
	{
		DirectX::XMFLOAT3 front;
		front.x = DirectX::XMScalarCos(Yaw) * DirectX::XMScalarCos(Pitch);
		front.y = DirectX::XMScalarSin(Pitch);
		front.z = DirectX::XMScalarSin(Yaw) * DirectX::XMScalarCos(Pitch);
		Direction = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&front));
	}
	
	void ProcessMouseMovement(float xoffset, float yoffset)
	{
		Yaw = DirectX::XMScalarModAngle(Yaw + xoffset);
		Pitch += yoffset;

		if (Pitch > 1.2f)
		{
			Pitch = 1.2f;
		}
		if (Pitch < -1.2f)
		{
			Pitch = -1.2f;
		}

		Orientation.Update({ -Pitch, -Yaw - DirectX::XMConvertToRadians(270) });

;		UpdateDirection();

		//DirectionXM = DirectX::XMVector3Rotate(DirectX::g_XMIdentityR2, Yquat);

	}
	void ProcessInput()
	{
		auto dTime = Time::Get().GetDeltaTimeMs();
		float velocity = Keyboard::IsKeyPressed(VK_SHIFT) ? 3.f : 1.f;
		if (Keyboard::IsKeyPressed('W'))
			Position = DirectX::XMVectorAdd(Position, DirectX::XMVectorScale(Direction, velocity * Speed * dTime));
		if (Keyboard::IsKeyPressed('S'))
			Position = DirectX::XMVectorAdd(Position, DirectX::XMVectorNegate(DirectX::XMVectorScale(Direction, velocity * Speed * dTime)));
		if (Keyboard::IsKeyPressed('A'))
			Position = DirectX::XMVectorAdd(Position, DirectX::XMVectorScale(RightVec, velocity * Speed * dTime));
		if (Keyboard::IsKeyPressed('D'))
			Position = DirectX::XMVectorAdd(Position, DirectX::XMVectorNegate(DirectX::XMVectorScale(RightVec, velocity * Speed * dTime)));
	}


private:
	float Yaw = 0.f;
	float Pitch = 0.f;
	float Speed = 0.1f;
	bool bLocked = false;

private://SaveToFile

	struct RFileData
	{
		float3 position;
		float2 yawPitch;
	};
	static constexpr wchar_t FileName[] = L"CameraState";

	void SaveToFile()
	{
		RFileData fileData;

		fileData.position = Position;
		fileData.yawPitch = { Yaw,Pitch };

		FileIO::TSaveToFile(&fileData, FileName);
	}

	void LoadFromFile()
	{
		RFileData fileData;

		FileIO::TLoadFromFile(&fileData, FileName);

		Position = fileData.position;
		Yaw = fileData.yawPitch.x;
		Pitch = fileData.yawPitch.y;
	}
};
