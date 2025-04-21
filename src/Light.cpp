#include "Light.h"
    void DirectionalLight::getIllumination(const Vector3f &p, 
                                 Vector3f &tolight, 
                                 Vector3f &intensity, 
                                 float &distToLight) const
    {
        // the direction to the light is the opposite of the
        // direction of the directional light source

        // BEGIN STARTER
        tolight = -_direction;
        intensity  = _color;
        distToLight = std::numeric_limits<float>::max();
        // END STARTER
    }
    void PointLight::getIllumination(const Vector3f &p, //点
                                 Vector3f &tolight, //指向光源的矢量
                                 Vector3f &intensity, //照明强度
                                 float &distToLight) const //场景点到光源的距离
    {
        // TODO Implement point light source
        // tolight, intensity, distToLight are outputs
        tolight = _position - p;
        distToLight = tolight.abs();
        tolight = tolight.normalized();
        intensity = _color / (_falloff * distToLight * distToLight);
    }
