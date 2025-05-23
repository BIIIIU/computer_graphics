#include "Object3D.h"

bool Sphere::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER

    // We provide sphere intersection code for you.
    // You should model other intersection implementations after this one.

    // Locate intersection point ( 2 pts )
    const Vector3f &rayOrigin = r.getOrigin(); //Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    Vector3f origin = rayOrigin - _center;      //Ray origin in the sphere coordinate

    float a = dir.absSquared();
    float b = 2 * Vector3f::dot(dir, origin);
    float c = origin.absSquared() - _radius * _radius;

    // no intersection
    if (b * b - 4 * a * c < 0) {
        return false;
    }

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f*a);
    float tminus = (-b - d) / (2.0f*a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin)) {
        return false;
    }

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin) {
        t = tminus;
    }

    // one intersection at the front. one at the back 
    if ((tplus > tmin) && (tminus < tmin)) {
        t = tplus;
    }

    if (t < h.getT()) {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal = normal.normalized();
        h.set(t, this->material, normal);
        return true;
    }
    // END STARTER
    return false;
}

// Add object to group
void Group::addObject(Object3D *obj) {
    m_members.push_back(obj);
}

// Return number of objects in group
int Group::getGroupSize() const {
    return (int)m_members.size();
}

bool Group::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER
    // we implemented this for you
    bool hit = false;
    for (Object3D* o : m_members) {
        if (o->intersect(r, tmin, h)) {
            hit = true;
        }
    }
    return hit;
    // END STARTER
}


Plane::Plane(const Vector3f &normal, float d, Material *m) : Object3D(m) {
    // TODO implement Plane constructor
    _normal = normal;
    _dist = d;
}
bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    // TODO implement
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
bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const 
{
    // TODO implement
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


Transform::Transform(const Matrix4f &m, Object3D *obj) : _object(obj) {
    // TODO implement Transform constructor
    _matrix = m;
}
bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    // TODO implement
    const Vector3f &rayOrigin = r.getOrigin(); // o
    const Vector3f &dir = r.getDirection(); // d

    Matrix4f m_1 = _matrix.inverse();
    Vector3f TransOrigin = (m_1 * Vector4f(rayOrigin, 1)).xyz(); // 射线的起点用逆矩阵变换，受平移影响
    Vector3f TransDir((m_1 * Vector4f(dir, 0)).xyz()); // 将射线的方向向量用逆矩阵变换到局部空间，不受平移影响，只受旋转/缩放影响
    Ray TransRay(TransOrigin, TransDir); // 构造在局部空间里的射线

    if(_object->intersect(TransRay, tmin * TransDir.abs(), h)){
        Vector3f normal = (m_1.transposed() * Vector4f(h.getNormal(), 0)).xyz(); // 把局部空间中求到的法线变回世界坐标，变换法线时需要使用逆转置矩阵
        normal = normal.normalized();
        h.set(h.getT(), h.getMaterial(), normal);
        return true;
    }
    return false;
}