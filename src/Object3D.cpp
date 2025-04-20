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
    Vector3f normal = Vector3f::cross(_v[1] - _v[0], _v[2] - _v[0]); // N
    normal = normal.normalized();

    if(Vector3f::dot(dir, normal) == 0){ // parallel
        return false;
    }

    float t = Vector3f::dot(_v[0] - rayOrigin, normal) / Vector3f::dot(dir, normal);
    if(t < tmin){
        return false;
    }
    if(t < h.getT()){
        Vector3f p = rayOrigin + t * dir;

        Vector3f n0 = Vector3f::cross(_v[1] - _v[0], p - _v[0]);
        Vector3f n1 = Vector3f::cross(_v[2] - _v[1], p - _v[1]);
        Vector3f n2 = Vector3f::cross(_v[0] - _v[2], p - _v[2]);
        float a = Vector3f::dot(n0, n1);
        float b = Vector3f::dot(n1, n2);

        if(a > 0 && b > 0){ // inside triangle
            h.set(t, this->material, normal);
            return true;
        }
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
    Vector3f TransDir((m_1 * Vector4f(dir, 0)).xyz()); // 将射线的方向向量用逆矩阵变换到局部空间，不受平移影响，只受旋转/缩放影响
    Ray TransRay((m_1 * Vector4f(rayOrigin, 1)).xyz(), TransDir); // 构造在局部空间里的射线，射线的起点用逆矩阵变换，受平移影响

    if(_object->intersect(TransRay, tmin * TransDir.abs(), h)){
        Vector3f normal = (m_1.transposed() * Vector4f(h.getNormal(), 0)).xyz(); // 把局部空间中求到的法线变回世界坐标，变换法线时需要使用逆转置矩阵
        normal = normal.normalized();
        h.set(h.getT(), h.getMaterial(), normal);
        return true;
    }
    return false;
}