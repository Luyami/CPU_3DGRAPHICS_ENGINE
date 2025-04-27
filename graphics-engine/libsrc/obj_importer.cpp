#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

#include "../include/stb_image.h"
#include "../include/obj_importer.h"

using namespace Obj_Importer;

ImportResult::ImportResult(std::vector<Vertex>* v, std::vector<UV>* t, std::vector<Face>* f, std::unordered_map<std::string, Material> m){
    this->vertices = v;
    this->texCoords = t;
    this->faces = f;
    this->materials = m;
}

ImportResult::~ImportResult(){
    delete(this->vertices);
    delete(this->texCoords);
    delete(this->faces);
}

Texture* Importer::loadTexture(const std::string& fileName){
        int width, height, channels;

        unsigned char* data = stbi_load(fileName.data(), &width, &height, &channels, 0);
        if (!data) {
            std::cerr << "Texture Import: Failed to load texture!" << fileName << std::endl;
            return nullptr;
        }

        Texture* texture = new Texture();
        texture->width = width;
        texture->height = height;
        texture->mat = new RGB[width * height];
    
        for (int y = 0; y < height; y++) {
            int flippedY = height - 1 - y; //Já que uso a convenção esquerda-baixo para as texturas UV
            
            for (int x = 0; x < width; x++) {
                int index = (flippedY * width + x) * channels;
                float r = (int)data[index]/255.0f;
                float g = (int)data[index + 1]/255.0f;
                float b = (int)data[index + 2]/255.0f;
                texture->mat[y * width + x] = RGB(r, g, b);
            }
        }
    
        stbi_image_free(data);

        return texture;
}

void Importer::loadMaterials(const std::string& mtlFilename, std::unordered_map<std::string, Material>& materials) {
    std::ifstream file(mtlFilename);
    if (!file.is_open()) {
        std::cerr << "Import Error: Could not open material file " << mtlFilename << std::endl;
        return;
    }

    std::string line;
    Material currentMaterial;

    while (std::getline(file, line)) {
        std::istringstream sstream(line);
        std::string prefix;
        sstream >> prefix;

        if (prefix == "newmtl") {
            if (!currentMaterial.name.empty()) {
                materials[currentMaterial.name] = currentMaterial;
            }
            sstream >> currentMaterial.name;
            currentMaterial.texturePath = "";
        } 
        else if (prefix == "Ns") {
            sstream >> currentMaterial.Ns;
        }
        else if (prefix == "Ka") {
            sstream >> currentMaterial.Ka[0] >> currentMaterial.Ka[1] >> currentMaterial.Ka[2];
        }
        else if (prefix == "Kd") {
            sstream >> currentMaterial.Kd[0] >> currentMaterial.Kd[1] >> currentMaterial.Kd[2];
        }
        else if (prefix == "Ks") {
            sstream >> currentMaterial.Ks[0] >> currentMaterial.Ks[1] >> currentMaterial.Ks[2];
        }
        else if (prefix == "map_Kd") {
            sstream >> currentMaterial.texturePath;
            currentMaterial.loadedTexture = loadTexture(currentMaterial.texturePath);
        }
    }

    if (!currentMaterial.name.empty()) {
        materials[currentMaterial.name] = currentMaterial;
    }
}

ImportResult* Importer::import(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Import error: Failed to open file");
    }

    std::vector<Vertex>* vertices = new std::vector<Vertex>();
    std::vector<UV>* texCoords = new std::vector<UV>();
    std::vector<Face>* faces = new std::vector<Face>();
    std::unordered_map<std::string, Material> materials;

    std::string currentMaterial;
    std::string mtlFilename;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream sstream(line);
        std::string prefix;
        sstream >> prefix;

        if (prefix == "mtllib") {  // Load material file
            sstream >> mtlFilename;
            mtlFilename = "importedObjects/" + mtlFilename;
            loadMaterials(mtlFilename, materials);
        }
        else if (prefix == "usemtl") {  // Assign material to face
            sstream >> currentMaterial;
        }
        else if (prefix == "v") {  // Vertex
            Vertex v;
            sstream >> v.x >> v.y >> v.z;
            vertices->push_back(v);
        }
        else if (prefix == "vt") {  // UV Coordinates
            UV tex;
            sstream >> tex.u >> tex.v;
            texCoords->push_back(tex);
        }
        else if (prefix == "f") {  // Face
            Face f;
            char slash;

            sstream >> f.v1 >> slash >> f.t1 >> slash >> f.n1;
            sstream >> f.v2 >> slash >> f.t2 >> slash >> f.n2;
            sstream >> f.v3 >> slash >> f.t3 >> slash >> f.n3;

            f.v1--; f.v2--; f.v3--;
            f.t1--; f.t2--; f.t3--;
            f.n1--; f.n2--; f.n3--;
        
            if (materials.count(currentMaterial) > 0){
                f.material = materials[currentMaterial];
            }
            else {f.material = Material(); f.material.isEmpty = true;}
            faces->push_back(f);
        }
    }

    ImportResult* r = new ImportResult(vertices, texCoords, faces, materials);

    return r;
}