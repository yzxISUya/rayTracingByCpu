#pragma once

#include "Walnut/Image.h"

#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

#include <memory>
#include <glm/glm.hpp>

//三件事：发射光线、碰撞检测、计算颜色
class Renderer
{
public:
	struct Settings
	{
		bool Accumulate = true;
	};
public:
	Renderer() = default;

	void OnResize(uint32_t width, uint32_t height);
	void Render(const Scene& scene, const Camera& camera);

	/*
	把渲染好的图像"借"给外部使用
	不能是裸指针，因为智会自动追踪有多少人在用这张图。Renderer自己一份，WalnutApp通过GetFinalImage()拿到另一份。任何一方销毁时不会影响另一方。裸的会悬空什么的吧，搞不懂，懒得管了
	*/
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& GetSettings() { return m_Settings; }
private:
	struct HitPayload
	{
		float HitDistance;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal; //法
		int ObjectIndex;
	};

	//渲染管线函数
	glm::vec4 PerPixel(uint32_t x, uint32_t y); //生成初始光线
	HitPayload TraceRay(const Ray& ray);//算交点
	HitPayload ClosestHit(const Ray& ray, float hitDistance, int objectIndex); //碰到了，算颜色
	HitPayload Miss(const Ray& ray); //用bool就不好搞扩展了，没碰到是返回天蓝色还是黑色呢？

private:
	std::shared_ptr<Walnut::Image> m_FinalImage;
	Settings m_Settings;

	//感觉这里和上面的智指有点像，这里只为了并行计算，for搞并行我得自己管理线程，但是for_each可以自动设置并行，nice
	std::vector<uint32_t> m_ImageHorizontalIter, m_ImageVerticalIter; //俩fvv数组，仅仅因为for_each不像for，居然只能塞迭代器，所以得造这两个数组，拿迭代器

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	uint32_t* m_ImageData = nullptr;//像素数组（rgba）
	glm::vec4* m_AccumulationData = nullptr;

	uint32_t m_FrameIndex = 1;
};