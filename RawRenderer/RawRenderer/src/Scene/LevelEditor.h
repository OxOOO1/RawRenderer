#pragma once

#include "ImGuiObject.h"
#include "thirdParty/ImGUI/imgui.h"
#include "DrawableComponent/Drawable.h"

class RLevelEditor
{
	static constexpr char ModelPathSphere[] = "assets/models/sphere.fbx";
	static constexpr char ModelPathCube[] = "assets/models/sphere.fbx";
	static constexpr char ModelPathPlane[] = "assets/models/sphere.fbx";
public:

	static void Render();


	static void CreateSphere();
	static void CreateCube();
	static void CreatePlane();


};