//主入口：组装场景、UI、渲染循环
#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "Renderer.h"
#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer()
		: m_Camera(45.0f, 0.1f, 100.0f)
	{
		// === 材质 ===

		// 前排：金属系
		Material& redMetal = m_Scene.Materials.emplace_back();
		redMetal.Albedo = { 0.9f, 0.1f, 0.1f };
		redMetal.Roughness = 0.2f;
		redMetal.Metallic = 1.0f;

		Material& blueDiffuse = m_Scene.Materials.emplace_back();
		blueDiffuse.Albedo = { 0.2f, 0.3f, 1.0f };
		blueDiffuse.Roughness = 0.6f;

		Material& greenMetal = m_Scene.Materials.emplace_back();
		greenMetal.Albedo = { 0.1f, 0.9f, 0.3f };
		greenMetal.Roughness = 0.1f;
		greenMetal.Metallic = 1.0f;

		// 中排：彩色漫反射
		Material& yellowMat = m_Scene.Materials.emplace_back();
		yellowMat.Albedo = { 1.0f, 0.9f, 0.1f };
		yellowMat.Roughness = 0.8f;

		Material& purpleMat = m_Scene.Materials.emplace_back();
		purpleMat.Albedo = { 0.6f, 0.1f, 0.9f };
		purpleMat.Roughness = 0.5f;

		Material& cyanMat = m_Scene.Materials.emplace_back();
		cyanMat.Albedo = { 0.1f, 0.9f, 0.9f };
		cyanMat.Roughness = 0.4f;

		// 后排：混合
		Material& silverMetal = m_Scene.Materials.emplace_back();
		silverMetal.Albedo = { 0.7f, 0.7f, 0.7f };
		silverMetal.Roughness = 0.05f;
		silverMetal.Metallic = 1.0f;

		Material& bronzeMetal = m_Scene.Materials.emplace_back();
		bronzeMetal.Albedo = { 0.8f, 0.5f, 0.2f };
		bronzeMetal.Roughness = 0.3f;
		bronzeMetal.Metallic = 1.0f;

		Material& whiteMat = m_Scene.Materials.emplace_back();
		whiteMat.Albedo = { 0.95f, 0.95f, 0.95f };
		whiteMat.Roughness = 0.7f;

		// 太阳：自发光
		Material& sunMat = m_Scene.Materials.emplace_back();
		sunMat.Albedo = { 1.0f, 0.6f, 0.1f };
		sunMat.EmissionColor = { 1.0f, 0.6f, 0.1f };
		sunMat.EmissionPower = 5.0f;

		// 地面
		Material& groundMat = m_Scene.Materials.emplace_back();
		groundMat.Albedo = { 0.5f, 0.5f, 0.5f };
		groundMat.Roughness = 0.9f;

		// === 球体 ===
		// 三排球，每排三个，沿 -Z 方向排列，深度不同
		// 前排 z=0（离相机最近）
		// 中排 z=-3
		// 后排 z=-6

		auto addSphere = [&](glm::vec3 pos, float radius, int matIdx) {
			Sphere s;
			s.Position = pos;
			s.Radius = radius;
			s.MaterialIndex = matIdx;
			m_Scene.Spheres.push_back(s);
		};

		// 前排
		addSphere({ -1.8f, 0.0f,  0.0f }, 0.6f, 0);  // 红金属
		addSphere({  0.0f, 0.0f,  0.0f }, 0.6f, 1);  // 蓝漫反射
		addSphere({  1.8f, 0.0f,  0.0f }, 0.6f, 2);  // 绿金属

		// 中排
		addSphere({ -1.8f, 0.0f, -3.0f }, 0.6f, 3);  // 黄
		addSphere({  0.0f, 0.0f, -3.0f }, 0.6f, 4);  // 紫
		addSphere({  1.8f, 0.0f, -3.0f }, 0.6f, 5);  // 青

		// 后排
		addSphere({ -1.8f, 0.0f, -6.0f }, 0.6f, 6);  // 银金属
		addSphere({  0.0f, 0.0f, -6.0f }, 0.6f, 7);  // 铜金属
		addSphere({  1.8f, 0.0f, -6.0f }, 0.6f, 8);  // 白漫反射

		// 太阳：悬空发光球
		addSphere({ 1.5f, 4.5f, -1.0f }, 0.8f, 9);

		// 地面
		addSphere({ 0.0f, -100.5f, 0.0f }, 100.0f, 10);

		// 相机初始位置和景深默认值
		m_Camera.SetPosition({ 0.0f, 1.5f, 4.0f });
		m_Camera.SetFocusDistance(5.0f);
		m_Camera.SetAperture(0.0f);
	}

	virtual void OnUpdate(float ts) override
	{
		if (m_Camera.OnUpdate(ts))
			m_Renderer.ResetFrameIndex();
	}

	//ImGui界面：Settings面板 + Scene面板 + Viewport
	virtual void OnUIRender() override
	{
		//Settings面板：渲染时间、累加开关、重置按钮
		ImGui::Begin("Settings");
		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}

		ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);

		if (ImGui::Button("Reset"))
			m_Renderer.ResetFrameIndex();

		float aperture = m_Camera.GetAperture();
		if (ImGui::DragFloat("Aperture", &aperture, 0.01f, 0.0f, 5.0f))
		{
			m_Camera.SetAperture(aperture);
			m_Renderer.ResetFrameIndex();
		}

		float focusDist = m_Camera.GetFocusDistance();
		if (ImGui::DragFloat("Focus Distance", &focusDist, 0.1f, 0.1f, 100.0f))
		{
			m_Camera.SetFocusDistance(focusDist);
			m_Renderer.ResetFrameIndex();
		}

		ImGui::End();

		//Scene面板：拖拽调整球体和材质参数
		ImGui::Begin("Scene");
		for (size_t i = 0; i < m_Scene.Spheres.size(); i++)
		{
			ImGui::PushID(i);

			Sphere& sphere = m_Scene.Spheres[i];
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
			ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
			ImGui::DragInt("Material", &sphere.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);

			ImGui::Separator();

			ImGui::PopID();
		}

		for (size_t i = 0; i < m_Scene.Materials.size(); i++)
		{
			ImGui::PushID(i);

			Material& material = m_Scene.Materials[i];
			ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
			ImGui::DragFloat("Roughness", &material.Roughness, 0.05f, 0.0f, 1.0f);
			ImGui::DragFloat("Metallic", &material.Metallic, 0.05f, 0.0f, 1.0f);
			ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
			ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.05f, 0.0f, FLT_MAX);

			ImGui::Separator();

			ImGui::PopID();
		}

		ImGui::End();

		//Viewport：渲染结果在这里显示，padding=0让图像贴边
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;

		auto image = m_Renderer.GetFinalImage();
		if (image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() },
				ImVec2(0, 1), ImVec2(1, 0)); //UV翻转因为图像y轴和ImGui相反

		ImGui::End();
		ImGui::PopStyleVar();

		Render(); //每帧都渲染
	}

	void Render()
	{
		Timer timer;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera);

		m_LastRenderTime = timer.ElapsedMillis(); //计时，显示在Settings面板
	}
private:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	float m_LastRenderTime = 0.0f;
};

//Walnut框架的入口：创建应用、推入Layer、设置菜单栏
Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}