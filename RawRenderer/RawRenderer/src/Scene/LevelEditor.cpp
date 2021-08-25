#include "LevelEditor.h"

#include "Scene\Scene.h"

RLevelEditorObject* SelectedObject = nullptr;

void RLevelEditor::Render()
{
	
	RScene::Get().GetCamera().RenderUIGlobal();
	RScene::Get().GlobalLight.RenderUIGlobal();

	ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("LevelEditor"))
	{
		if (ImGui::Button("CreateSphere"))
		{
			RScene::Get().DefaultDrawables.at(0).CreateNewInstance();
			auto& InstancedDrawable = RScene::Get().DefaultDrawables.at(0);
			SelectedObject = &InstancedDrawable.GetLastInstance();
		}
		ImGui::SameLine();
		if (ImGui::Button("CreateCube"))
		{
			RScene::Get().DefaultDrawables.at(1).CreateNewInstance();
			auto& InstancedDrawable = RScene::Get().DefaultDrawables.at(1);
			SelectedObject = &InstancedDrawable.GetLastInstance();
		}
		ImGui::SameLine();
		if (ImGui::Button("CreatePlane"))
		{
			RScene::Get().DefaultDrawables.at(2).CreateNewInstance();
			auto& InstancedDrawable = RScene::Get().DefaultDrawables.at(2);
			SelectedObject = &InstancedDrawable.GetLastInstance();
		}
		ImGui::SameLine();
		if (ImGui::Button("CreateSpotLight"))
		{
			RScene::CreateLight(LIGHT_TYPE_SPOT);
			SelectedObject = &RScene::Get().Lights.back();
		}
		ImGui::SameLine();
		if (ImGui::Button("CreatePointLight"))
		{
			RScene::CreateLight(LIGHT_TYPE_POINT);
			SelectedObject = &RScene::Get().Lights.back();
		}

		// Left
		{
			ImGui::BeginChild("left pane", ImVec2(100, 0), true);
			//Meshes
			//ImGui::BeginChildFrame()
			if (ImGui::BeginTabBar("Scene"))
			{
				if (ImGui::BeginTabItem("Global"))
				{
					RScene::Get().GetCamera().Selectable(SelectedObject);
					RScene::Get().GlobalLight.Selectable(SelectedObject);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Models"))
				{
					for (int i = 0; i < RScene::Get().DefaultDrawables.size(); i++)
					{
						auto& instancedDrawable = RScene::Get().DefaultDrawables.at(i);
						auto& Instances = instancedDrawable.DrawableInstances;
						for (int i = 0; i < Instances.size(); i++)
						{
							if (ImGui::Selectable(Instances.at(i).Name.c_str(), SelectedObject == &Instances.at(i)))
							{
								SelectedObject = &Instances.at(i);
							}
						}
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Lights"))
				{

					for (int i = 0; i < RScene::Get().Lights.size(); i++)
					{
						auto& light = RScene::Get().Lights.at(i);
						if (ImGui::Selectable(light.Name.c_str(), SelectedObject == &light))
						{
							SelectedObject = &light;
						}
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
			
			ImGui::EndChild();
		}
		ImGui::SameLine();

		// Right
		{
			if (SelectedObject)
			{
				SelectedObject->RenderUI();
			}
		}
	}
	ImGui::End();

	//RProfileEvent::ShowProfileUI();
}
