#include <math.h>

#include "../include/raycast.h"
#include "../include/datatypes.h"
#include "../include/objects.h"

//Classe RaycastHit
RaycastHit::RaycastHit() 
    : hitPoint(Vector3(0, 0, 0)), normal(Vector3(0, 0, 0)), object(nullptr), functionParameter(-1), hit(false) {}

RaycastHit::RaycastHit(Vector3 hitPoint, Vector3 normal, Object* object, float functionParameter, bool hit)
    : hitPoint(hitPoint), normal(normal), object(object), functionParameter(functionParameter), hit(hit) {}

//Classe Ray
Ray::Ray(Vector3 initialPos, Vector3 direction) 
    : initialPos(initialPos), direction(direction.normalize()) {}

Vector3 Ray::getInitialPos() { 
    return initialPos; 
}

Vector3 Ray::getDirection() { 
    return direction; 
}