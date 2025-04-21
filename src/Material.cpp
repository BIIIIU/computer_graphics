#include "Material.h"
#include <iostream> 
using namespace std;
Vector3f Material::shade(const Ray &ray,//相机光线
    const Hit &hit,//命中点
    const Vector3f &dirToLight,
    const Vector3f &lightIntensity)
{
    // TODO implement Diffuse and Specular phong terms
    //diffuse
    Vector3f L = dirToLight.normalized(); //光源方向
    Vector3f N = hit.getNormal().normalized(); //法线方向
    float clamped_dot = std::max(0.0f, Vector3f::dot(L, N)); 
    Vector3f diffuse = _diffuseColor * lightIntensity * clamped_dot; //漫反射

    //specular
    Vector3f V = -ray.getDirection().normalized(); //相机方向,与光线方向相反
    Vector3f R = (2 * Vector3f::dot(L, N) * N - L).normalized(); //反射方向,R = 2(N·L)N - L
    float specular_dot = std::max(0.0f, Vector3f::dot(V, R)); //反射光线与相机光线的夹角
    float tmp = pow(specular_dot, _shininess); 
    Vector3f specular = _specularColor * lightIntensity * tmp; //镜面反射

    return diffuse + specular; //返回最终颜色

}