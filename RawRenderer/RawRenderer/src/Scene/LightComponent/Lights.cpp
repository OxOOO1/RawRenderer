#include "Lights.h"

#include "thirdParty/ImGUI/imgui.h"
#include "Scene.h"

#include "DrawableComponent/DebugDraw.h"
#include "DrawableComponent/Drawable.h"

void RGlobalLight::RenderUI()
{
	auto& LightDesc = RScene::Get().SceneUniformBufferCPU.GlobalLightDesc;

	ImGui::ColorEdit3("Color", &LightDesc.Color.x);
	ImGui::DragFloat("Intensity", &LightDesc.Intensity);
	ImGui::SliderFloat("AmbientLightIntensity", &LightDesc.AmbientLightIntensity, 0.f, 0.5f);

	bool bRotated = false;

	if (ImGui::SliderAngle("Pitch", &RotationPitchYawRoll.x, 0, 360))
	{
		bRotated = true;
	}
	if (ImGui::SliderAngle("Yaw", &RotationPitchYawRoll.y, 0, 360))
	{
		bRotated = true;
	}
	if (ImGui::SliderAngle("Roll", &RotationPitchYawRoll.z, 0, 360))
	{
		bRotated = true;
	}

	if (bRotated)
	{
		Position = DirectX::XMVector3Rotate(DirectX::g_XMIdentityR1, RQuaternion{ RotationPitchYawRoll });
	}

	ImGui::Checkbox("InWindow", &bDrawInSeparateWindow);

}

RLight::RLight(uint LightType /*= LIGHT_TYPE_POINT*/)
{
	Desc.Type = LightType;
	Desc.Radius = 100;
	Desc.Color = { 1.f, 1.f, 1.f };
	Desc.Direction = { 0, -1, 0 };
	Desc.Intensity = 1.f;
	Desc.SpotlightAngles = { 0.7f,0 }; //x = cos(Inner); y = cos(Outer);

	bNeedsGpuUpload = true;

	/*if (LightType == LIGHT_TYPE_POINT)
	{
		DebugDrawInstance.Type = RDebugDrawable::UVSPHERE;
	}
	else
	{
		DebugDrawInstance.Type = RDebugDrawable::CIRCLE;
	}
	DebugDrawInstance.Index = RDebugDrawManagerS::CreateInstance(DebugDrawInstance.Type);*/
}

RLightSource::RLightSource(uint LightType /*= LIGHT_TYPE_POINT*/) : Light(LightType)
{
	switch (LightType)
	{
	case LIGHT_TYPE_POINT:
		IndexToDrawable = PointLightDrawable.CreateNewInstance();
		break;
	case LIGHT_TYPE_SPOT:
		IndexToDrawable = SpotLightDrawables.CreateNewInstance();
		break;
	default:
		break;
	}

	auto& underlyingDrawable = GetDrawableInstance();
	Name = underlyingDrawable.Name;
	underlyingDrawable.IndexToMaterial = IndexToMainMaterial;
	underlyingDrawable.Transform.Scale = { 0.2f, 0.2f, 0.2f };
}

void RLightSource::RenderUI()
{
	ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us

	ImGui::Separator();

	if (ImGui::Button("Go To"))
	{
		RScene::Get().GetCamera().GoTo(Light.Desc.Position, Light.Desc.Radius);
	}
	ImGui::SameLine();

	if (ImGui::Checkbox("AttachToCam", &Light.bAttachedToCam))
	{
		//GetDrawableInstance().bAttachedToCam = Light.bAttachedToCam;
	}

	if (ImGui::DragFloat("Radius", &Light.Desc.Radius, 1.f, 1.f, 500.f))
	{
		Light.bNeedsGpuUpload = true;
	}

	if (ImGui::DragFloat3("Position", &Light.Desc.Position.x))
	{
		Light.bNeedsGpuUpload = true;
		GetDrawableInstance().Transform.SetPosition(Light.Desc.Position);
	}

	if (ImGui::ColorEdit3("Color", &Light.Desc.Color.x))
	{
		Light.bNeedsGpuUpload = true;
		GetMaterial().SetColor(Light.Desc.Color);
	}

	if (ImGui::SliderFloat("Intensity", &Light.Desc.Intensity, 1.f, 100.f))
	{
		Light.bNeedsGpuUpload = true;
		auto& material = GetMaterial();
		material.Desc.Color.w = Light.Desc.Intensity;
		material.bNeedsGpuUpload = true;
	}

	ImGui::EndChild();
}

RDrawableInstance& RLightSource::GetDrawableInstance()
{
	switch (Light.Desc.Type)
	{
	case LIGHT_TYPE_POINT:
		return PointLightDrawable.GetInstance(IndexToDrawable);
		break;
	case LIGHT_TYPE_SPOT:
		return SpotLightDrawables.GetInstance(IndexToDrawable);
		break;
	default:
		assert(false);
		break;
	}
}

RMaterial& RLightSource::GetMaterial()
{
	return RMaterialManagerS::GetMaterial(GetDrawableInstance().IndexToMaterial);
}

void RLightSource::OnUpdate()
{
	Light.OnUpdate();
	if (Light.bAttachedToCam)
	{
		GetDrawableInstance().Transform.SetPosition(Light.Desc.Position);
	}
}

void RLightSource::SetPosition(const float3& position)
{
	Light.Desc.Position = position;
	Light.bNeedsGpuUpload = true;
	GetDrawableInstance().Transform.SetPosition(Light.Desc.Position);
}


void RLightSource::InitDrawablesAndMaterial()
{
	PointLightDrawable.Create("sphere");
	PointLightDrawable.Name = "PointLight";
	SpotLightDrawables.Create("sphere");
	SpotLightDrawables.Name = "SpotLight";

	IndexToMainMaterial = RMaterialManagerS::CreateNew("MainLightsMaterial", MAT_TYPE_EMITTER);
	RMaterialManagerS::GetMaterial(IndexToMainMaterial).Desc.Color = float4(1,1,1,1);
}

void RLightSource::UpdateLightDrawables()
{
	PointLightDrawable.OnUpdate();
	SpotLightDrawables.OnUpdate();
}

RDrawableMesh RLightSource::PointLightDrawable;

RDrawableMesh RLightSource::SpotLightDrawables;

UINT RLightSource::IndexToMainMaterial;

void RLight::OnUpdate()
{
	if (bAttachedToCam)
	{
		auto& cam = RScene::Get().GetCamera();
		Desc.Position = cam.GetPosition() + (cam.GetDirection() * Desc.Radius * 0.5f);
		bNeedsGpuUpload = true;
	}
	//if (bNeedsGpuUpload)
	//{
		/*//Debug Visualizer
		RDebugDrawable::InstanceDescription desc;
		desc.Color = Desc.Color;
		RTransformSimple transform;
		transform.Position = Desc.Position;
		desc.TransformMat = transform.GetTransformMatrix();
		RDebugDrawManagerS::UploadInstance(DebugDrawInstance, desc);*/

	//}
}
