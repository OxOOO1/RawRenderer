#include "Camera.h"



void RCamera::RenderUI()
{
	//if (!bDrawInSeparateWindow)
	ImGui::BeginChild("Camera", ImVec2(0, 350), true); // Leave room for 1 line below us
	
	float3 position = Position;
	ImGui::DragFloat3("Position", &position.x);
	float yp[2] = { Yaw, Pitch };
	ImGui::DragFloat2("yaw,pitch", yp);
	float3 direction = Direction;
	ImGui::DragFloat3("direction", &direction.x);
	bool PlanesChanged = false;
	PlanesChanged = ImGui::DragFloat("NearPlane", &NearPlane, 0.001f, 0.000001f, FarPlane - 1, "%.10f");
	if (PlanesChanged)
		UpdateProjectionMatrix();
	PlanesChanged = ImGui::DragFloat("FarPlane", &FarPlane, 1.f, NearPlane + 1, 2000.f);
	if (PlanesChanged)
		UpdateProjectionMatrix();
	
	ImGui::Checkbox("InWindow", &bDrawInSeparateWindow);

	//if(!bDrawInSeparateWindow)
	ImGui::EndChild();
}
