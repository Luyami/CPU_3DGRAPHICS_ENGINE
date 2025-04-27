#include <iostream>
#include <math.h>

#include "../include/datatypes.h"
#include "../include/objects.h"
#include <vector>

//Util
float clamp(float value, float min, float max){
    if (value < min) return min;
    if (value > max) return max;

    return value;
}

//Classe Vector3
Vector3::Vector3() : x(0), y(0), z(0) {}
Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

float Vector3::getX() const { 
    return x; 
}

float Vector3::getY() const { 
    return y; 
}

float Vector3::getZ() const { 
    return z; 
}

float Vector3::getAxisFromInt(int axis) const{
    if (axis == 0) return x;
    if (axis == 1) return y;
    if (axis == 2) return z;
}

float Vector3::cartesianDot(const Vector3& other) {
    return x * other.x + y * other.y + z * other.z;
}

Vector3 Vector3::cartesianCross(const Vector3& other){
    return Vector3(
        y * other.getZ() - z * other.getY(),
        z * other.getX() - x * other.getZ(),
        x * other.getY() - y * other.getX()
    );
}

float Vector3::magnitude() {
    return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
}

Vector3 Vector3::normalize() {
    return *this / magnitude();
}

std::ostream& operator<<(std::ostream& str, Vector3 const& o) { 
    str << '(' << o.x << ',' << o.y << ',' << o.z << ')';

    return str;
}
Vector3 operator+(const Vector3& o1, const Vector3& o2){
    return Vector3(o1.x + o2.x, o1.y + o2.y, o1.z + o2.z);
}
Vector3 operator-(const Vector3& o1, const Vector3& o2){
    return Vector3(o1.x - o2.x, o1.y - o2.y, o1.z - o2.z);
}
Vector3 operator-(const Vector3& o){
    return Vector3(-o.x, -o.y, -o.z);
}
Vector3 operator/(const Vector3& o1, const Vector3& o2){
    return Vector3(o1.x/o2.x, o1.y/o2.y, o1.z/o2.z);
}
Vector3 operator/(const Vector3& o, float n){
    return Vector3(o.x/n, o.y/n, o.z/n);
}
Vector3 operator*(const Vector3& o, float n){
    return Vector3(o.x*n, o.y*n, o.z*n);
}
Vector3 operator*(const Vector3& o1, const Vector3& o2){
    return Vector3(o1.x*o2.x, o1.y*o2.y, o1.z*o2.z);
}

Vector3::operator RGB() const{
    return RGB(x, y, z);
}

//Classe RGB
RGB::RGB(float R, float G, float B){
    this->R = clamp(R, 0, 1);
    this->G = clamp(G, 0, 1);
    this->B = clamp(B, 0, 1);
}
RGB::RGB() : RGB(1, 1, 1) {}

float RGB::getR() { 
    return R; 
}

float RGB::getG() { 
    return G; 
}

float RGB::getB() { 
    return B; 
}

RGB::operator Vector3() const { 
    return Vector3(R, G, B); 
}

//Classe MinimumPositive
MinimumPositive::MinimumPositive() {this->num = -1;}

float MinimumPositive::get() { return this->num; }

void MinimumPositive::insert(float newNum){
    if (newNum < 0.0f) return;

    if (this->num < 0.0f) {this->num = newNum; return;} //Caso em que ainda não foi inicializado 
    if (newNum < this->num) {this->num = newNum; return;}
}

bool MinimumPositive::hasPositive(){
    if (this->num >= 0.0f) return true;
    else return false;
}

//Struct Vertex
Vertex::operator Vector3() const {
    return Vector3(this->x, this->y, this->z);
}