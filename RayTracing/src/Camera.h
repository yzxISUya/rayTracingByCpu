#pragma once

#include <glm/glm.hpp>
#include <vector>

class Camera //计算每个像素对应的光线方向
{
public:
	Camera(float verticalFOV, float nearClip, float farClip);

	bool OnUpdate(float ts); //用户控制器，键鼠判定
	void OnResize(uint32_t width, uint32_t height);

	const glm::mat4& GetProjection() const { return m_Projection; }
	const glm::mat4& GetInverseProjection() const { return m_InverseProjection; }
	const glm::mat4& GetView() const { return m_View; }
	const glm::mat4& GetInverseView() const { return m_InverseView; }
	
	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetDirection() const { return m_ForwardDirection; }

	const std::vector<glm::vec3>& GetRayDirections() const { return m_RayDirections; }

	float GetRotationSpeed();
private:
	void RecalculateProjection();      //重算投影矩阵
	void RecalculateView();            //重算视图矩阵
	void RecalculateRayDirections();   //重算所有光线方向，最重要的一集
private:
	glm::mat4 m_Projection{ 1.0f };         //投影矩阵
	glm::mat4 m_View{ 1.0f };               //视图矩阵
	glm::mat4 m_InverseProjection{ 1.0f };  //投影矩阵的逆
	glm::mat4 m_InverseView{ 1.0f };        //视图矩阵的逆

	float m_VerticalFOV = 45.0f;     //垂直视野角度（Field of View）
	float m_NearClip = 0.1f;         //近裁剪面
	float m_FarClip = 100.0f;        //远裁剪面

	glm::vec3 m_Position{ 0.0f, 0.0f, 0.0f };      //相机在哪
	glm::vec3 m_ForwardDirection{ 0.0f, 0.0f, 0.0f }; //相机朝哪看

	// Cached ray directions
	std::vector<glm::vec3> m_RayDirections;  // 预计算的所有光线方向

	glm::vec2 m_LastMousePosition{ 0.0f, 0.0f };  // 上一帧鼠标位置
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
};
