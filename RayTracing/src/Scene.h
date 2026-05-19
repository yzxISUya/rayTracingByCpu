#pragma once
//球体，材质的定义
#include <glm/glm.hpp>
#include <vector>

class Material
{
public:
	glm::vec3 Albedo{ 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;
	glm::vec3 EmissionColor{ 0.0f };
	float EmissionPower = 0.0f;

	glm::vec3 GetEmission() const { return EmissionColor * EmissionPower; }//点乘
};

class Sphere
{
public:
	glm::vec3 Position{0.0f};
	float Radius = 0.5f;

	int MaterialIndex = 0;//使用的材质是哪个
};

class Scene
{
public:
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
};