#ifndef RAYCAST_H
#define RAYCAST_H

#include "../include/datatypes.h"
#include "../include/objects.h"

class Object;

class RaycastHit{
    public:
        bool hit;

        Object* object;

        float functionParameter;
        
        Vector3 hitPoint;
        Vector3 normal;

        int id = -1; //For meshes
        Vector3 baryCoords; //For triangles

        bool hasTexture = false;
        RGB textureColor; //texture color at hit point

        RaycastHit();
        RaycastHit(Vector3 hitPoint , Vector3 normal, Object* object, float functionParameter, bool hit);
};

class Ray{
    private:
        Vector3 initialPos;
        Vector3 direction;

    public:
        Ray(Vector3 initialPos, Vector3 direction);

        Vector3 getInitialPos();
        Vector3 getDirection();
};

#endif