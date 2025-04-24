# 计算机图形学Project2报告：光照模型与光线追踪

###### 陈雨妍 21307130449，张宇琪 21307130468

## 任务1: Phong光照模型

Phong光照模型由三个部分组成：

- 环境光照：均匀的背景照明
- 漫反射：光线在所有方向均匀散射
- 镜面反射：类似镜面的反射，产生高光效果

### 1.1 点光源实现

在`PointLight::getIllumination()`中，计算从一个点到光源的方向向量、该点处的光强度以及到光源的距离，根据公式计算即可：

```cpp
void PointLight::getIllumination(const Vector3f &p,
                               Vector3f &tolight,
                               Vector3f &intensity,
                               float &distToLight) const
{
    tolight = _position - p;
    distToLight = tolight.abs();
    tolight = tolight.normalized();
    intensity = _color / (_falloff * distToLight * distToLight);
}
```

### 1.2 Phong着色实现

Phong光照模型将光分为环境光、散射光(diffuse)和镜面光照(specular)，环境光部分不在当前函数实现

#### 1.2.1 漫反射

$L$表示光线方向，$N$表示法向量，$lightIntensity$表示光强，则漫反射强度为：

$$I_{diffuse}=clamp(L·N)*lightIntensity*k_{diffuse}$$

不用每个通道单独算，*就是按照元素乘

#### 1.2.2 镜面反射

视线方向$V$ 是ray的反方向，反射方向$R=(2*L·N)*N-L$，则镜面反射强度为：

$$I_{specular}=clamp(V·R)*lightIntensity*k_{specular}$$

```cpp
Vector3f Material::shade(const Ray &ray,
    const Hit &hit,
    const Vector3f &dirToLight,
    const Vector3f &lightIntensity)
{
    // 漫反射部分
    Vector3f L = dirToLight.normalized(); // 光源方向
    Vector3f N = hit.getNormal().normalized(); // 法线方向
    float clamped_dot = std::max(0.0f, Vector3f::dot(L, N)); 
    Vector3f diffuse = _diffuseColor * lightIntensity * clamped_dot;

    // 镜面反射部分
    Vector3f V = -ray.getDirection().normalized(); // 视线方向
    Vector3f R = (2 * Vector3f::dot(L, N) * N - L).normalized(); // 反射方向
    float specular_dot = std::max(0.0f, Vector3f::dot(V, R));
    float tmp = pow(specular_dot, _shininess); 
    Vector3f specular = _specularColor * lightIntensity * tmp;

    return diffuse + specular; // 组合照明
}
```

## 任务2: 光线投射



## 任务3: 光线追踪与阴影投射

在`traceRay()`中实现光线追踪和阴影投射，计算流程如下：

1. 检查光线是否与场景中的对象相交
2. 如果相交，根据光源计算颜色
   1. 颜色初始化为环境光，因为只算一次
   2. 计算直接光，对于场景中的每个光源：
      1. 计算交点处的光照强度
      2. 如果启用了阴影投射，向光源发送阴影光线，如果在光源之前有交点，说明当前点在阴影中，忽略来自该光源的直接照明
      3. 如果点不在阴影中，添加Phong着色贡献
   3. 如果启用了光线追踪，计算视线的反射方向，递归追踪该光线得到间接照明
3. 如果没有找到交点，返回背景颜色，并且需要将深度修改为最浅（虽然与光线没有交点，但是背景也不是无穷远）

[注] 阴影光线和视线反射光线的原点都需要稍微远离发出点，避免由于精度问题光线与交点相交

```cpp
Vector3f Renderer::traceRay(const Ray &r, float tmin, int bounces, Hit &h) const
{
    Vector3f Indirect_color = Vector3f(0.0f, 0.0f, 0.0f);
    
    if (_scene.getGroup()->intersect(r, tmin, h)) {
        Vector3f color = _scene.getAmbientLight() * h.getMaterial()->getDiffuseColor();
        
        for (int i = 0; i < _scene.getNumLights(); i++) {
            Vector3f tolight, intensity;
            float distToLight;
            _scene.getLight(i)->getIllumination(r.pointAtParameter(h.getT()), 
                                                tolight, intensity, distToLight);

            // 阴影投射
            if(_args.shadows) {
                Vector3f shadowRayOrigin = r.pointAtParameter(h.getT()) + tolight * 0.01;
                Ray shadowRay(shadowRayOrigin, tolight);
                Hit shadowHit = Hit();
                if (_scene.getGroup()->intersect(shadowRay, tmin, shadowHit)) {
                    if (shadowHit.getT() < distToLight) {
                        continue; // 对于这个光源，点处于阴影中
                    }
                }
            }
            color += h.getMaterial()->shade(r, h, tolight, intensity);
        }
        
        // 递归光线追踪用于反射
        if (bounces > 0) {
            Hit indirect_h;
            Vector3f N = h.getNormal().normalized(); 
            Vector3f V = -r.getDirection().normalized(); 
            Vector3f R = (2 * Vector3f::dot(V, N) * N - V).normalized();
            
            Ray Ray_R = Ray(r.pointAtParameter(h.getT()) + 0.01 * R, R.normalized());
            Vector3f indirect_color = traceRay(Ray_R, tmin, bounces - 1, indirect_h);
            color += indirect_color * h.getMaterial()->getSpecularColor();
        }
        return color;
    } else {
        h.t = std::numeric_limits<float>::min();
        return _scene.getBackgroundColor(r.getDirection());
    }
}
```

## 生成图形截图