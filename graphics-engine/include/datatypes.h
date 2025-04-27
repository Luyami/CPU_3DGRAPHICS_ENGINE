#ifndef DATATYPES_H
#define DATATYPES_H

#include <iostream>

#define __host__ __device__

class RGB;
class Vector3;

typedef struct AABB AABB;
typedef struct BVHNode BVHNode;

class Vector3{
    private:
        float x;
        float y;
        float z;

    public:
        Vector3();
        Vector3(float x, float y, float z);

        float getX() const;
        float getY() const;
        float getZ() const;

        float getAxisFromInt(int axis) const; //0 para X, 1 para Y, 2 para Z;

        float cartesianDot(const Vector3& other);
        Vector3 cartesianCross(const Vector3& other);

        float magnitude();

        Vector3 normalize();

        friend std::ostream& operator<<(std::ostream& str, Vector3 const& o);
        friend Vector3 operator+(const Vector3& o1, const Vector3& o2);
        friend Vector3 operator-(const Vector3& o1, const Vector3& o2);
        friend Vector3 operator-(const Vector3& o);
        friend Vector3 operator/(const Vector3& o1, const Vector3& o2);
        friend Vector3 operator/(const Vector3& o, float n);
        friend Vector3 operator*(const Vector3& o, float n);
        friend Vector3 operator*(const Vector3& o1, const Vector3& o2);

        operator RGB() const;
};

class RGB{
    private:
        float R;
        float G;
        float B;

    public:
        RGB(float R, float G, float B);
        RGB();

        float getR();
        float getG();
        float getB();

    operator Vector3() const;
};

typedef struct Texture{
    RGB* mat; //Texture matrix;
    int width;
    int height;
} Texture;

//Se o material não tiver nenhuma textura, os componentes de energia ambiente, difusa e especular são utilizados!
struct Material {
    bool isEmpty = false;

    std::string name;
    std::string texturePath;

    float specularExponent;
    RGB ambientEnergy;
    RGB diffuseEnergy;
    RGB specularEnergy;

    Texture* loadedTexture = nullptr;
};

struct UV {
    float u, v;
};

typedef struct Vertex{
    float x, y, z;

    operator Vector3() const;
} Vertex;

typedef struct Face{
    int v1, v2, v3;  // vertice indices
    int t1, t2, t3;  // uv indices
    int n1, n2, n3;  // normal indices
    Material material;
} Face;

//O valor que essa classe armazena é sempre o menor número positivo que foi colocado nela
class MinimumPositive{
    private:
        float num;
    
    public:
        MinimumPositive();

        float get();
        void insert(float num);
        bool hasPositive();
};

#endif 