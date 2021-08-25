#include "SceneObject.h"

#include "Scene/Scene.h"

#include "thirdParty/ImGUI/imgui.h"

void RLevelEditorObject::RenderUIGlobal()
{
	if (bDrawInSeparateWindow)
	{
		if (ImGui::Begin(Name.c_str()))
		{
			RenderUI();
			ImGui::End();
		}
	}
}

void RLevelEditorObject::Selectable(RLevelEditorObject*& pObject)
{
	if (ImGui::Selectable(Name.c_str()))
	{
		pObject = this;
	}
}
