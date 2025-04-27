#ifndef OBJIMPORTER_H
#define OBJIMPOTER_H

#include "../include/datatypes.h"

#include <vector>
#include <unordered_map>


namespace Obj_Importer{

struct Material {
    bool isEmpty = false;

    std::string name;
    float Ns;  // expoente da energia especular
    float Ka[3];  // Energia ambiente
    float Kd[3];  // Energia difusa
    float Ks[3];  // Energia especular
    std::string texturePath;
    Texture* loadedTexture = nullptr;
};

typedef struct Vertex{
    float x, y, z;
} Vertex;

typedef struct Face{
    int v1, v2, v3;  // vertice indices
    int t1, t2, t3;  // uv indices
    int n1, n2, n3;  // normal indices
    Material material;
} Face;

struct UV {
    float u, v;
};

class ImportResult{
    public:
        std::vector<Vertex>* vertices;
        std::vector<UV>* texCoords;
        std::vector<Face>* faces;
        std::unordered_map<std::string, Material> materials;

        ImportResult(std::vector<Vertex>* v, std::vector<UV>* t, std::vector<Face>* f, std::unordered_map<std::string, Material> m);
        ~ImportResult();
};

class Importer{
    private:
        static Texture* loadTexture(const std::string& fileName);
        static void loadMaterials(const std::string& mtlFilename, std::unordered_map<std::string, Material>& materials);

    public:
        static ImportResult* import(const std::string& filename);
};

}
#endif