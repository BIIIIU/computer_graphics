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

光线投射需要将光线追踪器变得灵活和可扩展。一个通用的Object3D类将作为所有三维物体的父类，需要实现的是它们专门化的子类，并实现相交（intersect）函数。实现相交函数需要通过求出相交距离t来更新Hit存储的法向量值。

### 2.1 实现平面（Plane）类

已知一个平面P，求出相交距离$$t=\frac{(p'-o)·N}{d·N}$$，其中`p'`是平面上的点， `o`是射线起点，`d`是射线方向，`N`是平面法向量。

如果发现点使得`t >= tmin`和`t < h.getT()`，则需要更新Hit，并且只要是相交就要返回true。

代码实现如下：

```cpp
bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    const Vector3f &rayOrigin = r.getOrigin(); // o
    const Vector3f &dir = r.getDirection(); // d

    Vector3f origin = _normal * _dist - rayOrigin; // p' - o

    float t = Vector3f::dot(origin, _normal) / Vector3f::dot(dir, _normal);
    if(t < tmin){
        return false;
    }
    if(t < h.getT()){
        h.set(t, this->material, _normal);
        return true;
    }
    return false;
}
```

### 2.2 实现三角形（Triangle）类

构造函数输入3个顶点，每个顶点包括法线和材质。我们需要做的是：

- 判断射线是否与三角形所在平面相交。
- 确定射线与平面相交后，求得其交点p，并判断该点是否在三角形内。

若点p满足上面两个条件，那么p一定满足方程$$p=(1-u-v)*v0+u*v1+v*v2$$（p在三角形上的重心坐标公式）和方程$$p=o+t*d$$（射线公式），联立两个方程用矩阵的方法就可以求解`[u, v, t]`了。

要保证p在三角形内部，则p的重心坐标公式三个系数都在0-1之间即可。同样的，如果发现点使得`t >= tmin`和`t < h.getT()`，则需要更新Hit，并且只要是相交就要返回true。

代码实现如下：

```cpp
bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const 
{
    const Vector3f &rayOrigin = r.getOrigin(); // o
    const Vector3f &dir = r.getDirection(); // d
    
    // 解(1-u-v)*v0 + u*v1 + v*v2 = o + t*d
    Matrix3f matrix(_v[1] - _v[0], _v[2] - _v[0], -dir);
    Vector3f ans = matrix.inverse() * (rayOrigin - _v[0]); // [u, v, t]

    if(1 - ans[0] - ans[1] > 0 && ans[0] > 0 && ans[1] > 0 && ans[2] >= tmin && ans[2] < h.getT()){
        Vector3f normal = (1 - ans[0] - ans[1]) * _normals[0] + ans[0] * _normals[1] + ans[1] * _normals[2];
        normal = normal.normalized();
        h.set(ans[2], this->material, normal);
        return true;
    }
    return false;
}
```

### 2.3 实现变换（Transform）类

变换类存储一个指向子对象三维节点的指针。它还存储了一个4x4的变换矩阵`_matrix`，这个矩阵将子对象从局部对象坐标移动到世界坐标。但是对于复杂的子对象，例如具有许多顶点的网格，每当我们想要跟踪一条射线时，我们不能将整个对象移动到世界坐标。

将光线从世界坐标移动到局部对象坐标要计算小得多，所以我们需要先将射线从世界空间变换到局部空间，在局部空间中进行与原始几何体的相交计算，最后将相交信息（特别是法线）变换回世界空间。

- 从世界坐标移动到局部对象坐标是从局部对象坐标移动到世界坐标的逆过程，所以变换矩阵是`_matrix.inverse()`。

- 射线起点作为点变换，使用齐次坐标 `(rayOrigin, 1)`，受平移影响；射线方向作为向量变换，使用齐次坐标 `(dir, 0)`，不受平移影响。

- 在局部空间中构造射线并判断是否相交，最后将相交信息变换回世界空间。

代码实现如下：

```cpp
bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    const Vector3f &rayOrigin = r.getOrigin(); // o
    const Vector3f &dir = r.getDirection(); // d

    Matrix4f m_1 = _matrix.inverse();
    Vector3f TransOrigin = (m_1 * Vector4f(rayOrigin, 1)).xyz();
    Vector3f TransDir((m_1 * Vector4f(dir, 0)).xyz());
    Ray TransRay(TransOrigin, TransDir);

    if(_object->intersect(TransRay, tmin * TransDir.abs(), h)){
        Vector3f normal = (m_1.transposed() * Vector4f(h.getNormal(), 0)).xyz();
        normal = normal.normalized();
        h.set(h.getT(), h.getMaterial(), normal);
        return true;
    }
    return false;
}
```



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

![a01](../out/a01.png)

![a01d](../out/a01d.png)

![a01n](../out/a01n.png)

![a02](../out/a02.png)

![a02d](../out/a02d.png)

![a02n](../out/a02n.png)

![a03](../out/a03.png)

![a03d](../out/a03d.png)

![a03n](../out/a03n.png)

![a04](../out/a04.png)

![a04d](../out/a04d.png)

![a04d](../out/a04n.png)

![a05](../out/a05.png)

![a05d](../out/a05d.png)

![a05n](../out/a05n.png)

![a06](../out/a06.png)

![a06d](../out/a06d.png)

![a06n](../out/a06n.png)

![a07](../out/a07.png)

![a07d](../out/a07d.png)

![a07n](../out/a07n.png)