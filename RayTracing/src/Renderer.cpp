#include "Renderer.h"
#include "Walnut/Random.h"
#include <execution>

namespace Utils {

	//gamma校正，依旧肉眼问题
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(glm::sqrt(color.r) * 255.0f);
		uint8_t g = (uint8_t)(glm::sqrt(color.g) * 255.0f);
		uint8_t b = (uint8_t)(glm::sqrt(color.b) * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r; //按RGBA顺序打包
		return result;
	}

}

//在单位圆盘内均匀采样（用于景深的光圈偏移）
static glm::vec2 RandomInUnitDisk()
{
	float angle = Walnut::Random::Float() * 2.0f * 3.14159265f;
	float r = glm::sqrt(Walnut::Random::Float());
	return glm::vec2(r * glm::cos(angle), r * glm::sin(angle));
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	//重建像素缓冲区和累加缓冲区
	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	//给for_each并行用
	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIter[i] = i;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	//第一帧清空累加缓冲，后续帧在上面叠加
	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

#define MT 1
#if MT
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this, y](uint32_t x)
				{
					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

					glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedColor /= (float)m_FrameIndex;

					accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
				});
		});

#else

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

			glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulatedColor /= (float)m_FrameIndex;

			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
		}
	}
#endif

	m_FinalImage->SetData(m_ImageData); //把像素数据传给GPU显示

	//累加模式：帧数越多噪点越少，关了就每帧重新算（会闪）
	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

//核心：每个像素走一遍路径追踪
glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	//从相机发射光线
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	//景深：偏移光线起点（模拟透镜孔径），方向指向焦平面上的交点
	float aperture = m_ActiveCamera->GetAperture();
	if (aperture > 0.0f)
	{
		float focusDist = m_ActiveCamera->GetFocusDistance();
		glm::vec3 focalPoint = ray.Origin + ray.Direction * focusDist;

		glm::vec3 right = glm::vec3(m_ActiveCamera->GetInverseView()[0]);
		glm::vec3 up = glm::vec3(m_ActiveCamera->GetInverseView()[1]);

		glm::vec2 offset = RandomInUnitDisk() * aperture;
		ray.Origin += right * offset.x + up * offset.y;
		ray.Direction = glm::normalize(focalPoint - ray.Origin);
	}

	glm::vec3 light(0.0f);       //累计的光颜色
	glm::vec3 contribution(1.0f); //衰减系数，每次弹射乘以albedo

	int bounces = 4; //最多弹射5次
	for (int i = 0; i < bounces; i++)
	{
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f) //没碰到任何东西→天空
		{
			//根据光线方向y分量做天空渐变（白→蓝）
			glm::vec3 unitDir = glm::normalize(ray.Direction);
			float t = 0.5f * (unitDir.y + 1.0f);
			glm::vec3 skyColor = (1.0f - t) * glm::vec3(1.0f) + t * glm::vec3(0.5f, 0.7f, 1.0f);
			light += skyColor * contribution;
			break;
		}

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		light += material.GetEmission(); //自发光直接加上去

		//金属和漫反射的弹射策略不同
		if (material.Metallic >= 0.5f)
		{
			//金属：镜面反射，Roughness越大反射越模糊
			ray.Direction = glm::reflect(ray.Direction,
				payload.WorldNormal + material.Roughness * Walnut::Random::InUnitSphere());
			contribution *= material.Albedo;
		}
		else
		{
			//漫反射：随机方向弹射（Lambertian）
			contribution *= material.Albedo;
			ray.Direction = glm::normalize(payload.WorldNormal + Walnut::Random::InUnitSphere());
		}

		//偏移一点点避免自相交（shadow acne问题）
		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
	}

	return glm::vec4(light, 1.0f);
}

//光线-球体求交：把光线方程代入球方程，解一元二次方程
Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	// |origin + t*direction|² = radius²
	// 展开成 at² + bt + c = 0
	// a = dot(dir,dir), b = 2*dot(origin,dir), c = dot(origin,origin) - r²

	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();
	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position; //把坐标系移到球心

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discriminant = b * b - 4.0f * a * c; //判别式
		if (discriminant < 0.0f)
			continue; //无实根=没碰到

		//取较小的根（近的那个交点），较大的t0是穿出点，暂时不用
		// float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a);
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT > 0.0f && closestT < hitDistance) //t>0保证交点在光线前方
		{
			hitDistance = closestT;
			closestSphere = (int)i;
		}
	}

	if (closestSphere < 0)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphere);
}

//碰到了：算出交点位置和法线
Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	//先在球心坐标系算交点，法线就是交点方向（球的法线=从球心指向交点）
	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position; //移回世界坐标

	return payload;
}

//没碰到：HitDistance设-1标记miss
Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}
