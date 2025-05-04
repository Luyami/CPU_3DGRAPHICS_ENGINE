#ifndef OBJECTS_H
#define OBJECTS_H

#include <iostream>
#include <vector>
#include <math.h>
#include <stack>

#include "../../sdl/sdl/include/SDL.h"
#include "../../glew/glew/include/GL/glew.h"

#include "../include/datatypes.h"
#include "../include/raycast.h"
    
using namespace std;

class RaycastHit;
class Scene;
class Camera;
class Ray;
class Mesh;
class Triangle;

typedef struct AABB AABB;
typedef struct BVHNode BVHNode;

enum ObjType{
    TSphere,
    TSurface,
    TCylinder,
    TCone,
    TTriangle,
    TMesh,
};

enum RenderizationMode{
    Default,
    Wireframe,
};

enum TextureFiltering{
    Nearest,
    Bilinear,
};

enum CameraProjection{
    Perspective,
    Orthographic,
    Oblique,
};

enum LightType{
    Point,
    Directional,
    Spot,
};

class Object{
    protected:
        Scene* s_owner = nullptr;

        string name;
        Vector3 position;

        RGB ambientColor;
        RGB diffuseColor;
        RGB specularColor;
        float specularExponent;
        float reflectivity; //0: nenhuma refletividade, 1: espelho perfeito

    public:
        ObjType type;

        Object();
        Object(string name, Vector3 position, RGB ambientColor, RGB diffuseColor, RGB specularColor, ObjType type);

        virtual RGB getAmbientColor();
        virtual RGB getDiffuseColor();
        virtual RGB getSpecularColor();
        virtual float getSpecularExponent();

        virtual void setReflectivity(float reflectivity);
        virtual float getReflectivity();

        void changeColor(RGB newColor);

        virtual Vector3 getPosition();

        virtual void move(Vector3 dir);

        void setSceneOwnership(Scene* s);

        void setName(string newName);
        string getName();

        virtual Vector3 getAABBminCoords() = 0;
        virtual Vector3 getAABBmaxCoords() = 0;

        virtual RaycastHit getRaycastHit(Ray ray) = 0;
        virtual Vector3 getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay) = 0;
};

class Light{
    private:
        LightType type;
        string name;

        bool on;

    public:
        //p/ tipos gerais
        Vector3 position;
        Vector3 direction;

        //p/ spot
        float spotAngle;
        float focus;

        //p/ point
        float potency;

        RGB colorRGB;

        Light();
        Light(string name);
        Light(string name, Vector3 position);
        Light(string name, Vector3 position, RGB colorRGB);

        void move(Vector3 dir);

        string getName();
        void setName(string newName);

        RGB getColor();
        void setColor(RGB colorRGB);

        string typeAsString();

        Vector3 getPosition();
        LightType getType();

        bool isOn();
        void turnOn();
        void turnOff();

        float& getPotency_ref();
        float& getSpotAngle_ref();
        float& getFocus_ref();
        bool& getOn_ref();

        static Light* pointLight(Vector3 position, RGB colorRGB, float potency, string name);
        static Light* pointLight(Vector3 position, RGB colorRGB, float potency);

        static Light* directionalLight(Vector3 direction, RGB colorRGB, string name);
        static Light* directionalLight(Vector3 direction, RGB colorRGB);
        
        static Light* spotLight(Vector3 position, Vector3 direction, float spotAngl, float focus, RGB colorRGB, string name); //spotAngle em graus
        static Light* spotLight(Vector3 position, Vector3 direction, float spotAngl, float focus, RGB colorRGB); 

        bool __isShadowed_gpu(Triangle *objs, BVHNode *bvh, RaycastHit hit);
        bool isShadowed(Scene& scene, RaycastHit hit);

        RGB __evaluateShading_gpu(Triangle *objs, BVHNode *bvh, Vector3 *eyePos, RaycastHit hit);
        RGB evaluateShading(Scene& scene, RaycastHit hit);
};

class Frame{
    private:
        Vector3 FRONT_DIR = Vector3(0, 0, -1);
        Vector3 UP_DIR = Vector3(0, 1, 0); //Eixo que tem direção "para cima" no frame
        Vector3 RIGHT_DIR = Vector3(1, 0, 0); //Eixo que tem direção "para direita" no frame

        //esses valores são imutáveis independente da projeção
        float default_width;
        float default_height;

        float width; 
        float height;

        int columns;
        int rows;

        float dX; //Largura do pixel
        float dY; //Altura do pixel

        Vector3 position;
        Vector3 camPosition;

        void rotateXZ(float degrees, float dFrame);
        void rotateY(float degrees, float dFrame);
    public:
        Frame(float width, float height, int columns, int rows);
        Frame();

        void move(Vector3 dir);
        void moveTo(Vector3 newPos);

        float getDefaultWidth();
        float getDefaultHeight();

        float getWidth();
        float getHeight();

        void setWidth(float width);
        void setHeight(float height);

        int getColumns();
        int getRows();

        Vector3 getPosition();
        Vector3 getCamPosition();

        Vector3 getFrontDir();
        Vector3 getRightDir();
        Vector3 getUpDir();

        Vector3 getPixelPos(int row, int col);

        friend class Camera;
};

class Camera{
    private:
        Vector3 eyePos;

        Frame frame;
        float dFrame;
    public:
        Camera(Vector3 eyePos, Frame frame, float dFrame);
        Camera();

        //Parâmetros para projeção oblíqua
        float oblique_frame_scale;
        float oblique_inclination_in_degrees;


        //Parâmetros para projeção ortogonal
        float orthogonal_frame_scale;

        Vector3 getEyePos();
        Vector3 getFrameCameraPosition();

        Frame& getFrame();

        void move(Vector3 dir);
        void LookAt(Vector3 pos);

        void rotateXZ(float degrees);
        void rotateY(float degrees);

        void changeVerticalFov(float degrees);

        friend class Frame;
        friend class Scene;
};

class Scene{
    private:
        vector<Object*> plain_objects = vector<Object*>(); //Armazena os objetos de forma plana (ex: armazena todos triângulos de uma malha)

        vector<Object*> objects = vector<Object*>();
        vector<Light*> lightSources = vector<Light*>();

        void cacheBVH();
        BVHNode* cachedBVH;

        Camera camera;
        SDL_Window* canvas;

        void drawScene();

        void updateFrameProportions(); //Atualiza as proporções do frame de acordo com a projeção escolhida!

        RGB computeShading(RaycastHit hit, RGB AMBIENT_ENERGY);
    public:
        static RenderizationMode mode;
        static TextureFiltering texFiltering;
        static CameraProjection camProjection;

        static int rt_rays;
        static int optimizationLevel;
        static bool optimizedRenderization;
        static bool should_predict_pixel;

        Scene(Camera camera, SDL_Window* canvas);
        Scene(Camera camera); //Esse construtor espera que o canvas seja setado por meio da função setCanvas()

        void setCanvas(SDL_Window* canvas);

        void addObject(Object* o);
        void addLight(Light* l);

        void removeObject(Object* o);

        void requestBVHCaching();

        vector<Object*> getObjects();
        vector<Light*> getLightSources();
        SDL_Window* getCanvas();

        Object* pick(int x, int y); //x do pixel e y do pixel
        Camera& getCamera();

        RaycastHit Raycast(Ray rayInfo);

        static void switchOptimizedRenderization();
        static void changeTextureFiltering(TextureFiltering filter);
        static void changeRenderizationMode(RenderizationMode mode);
        static void changeCameraProjection(CameraProjection projection);

        void renderScene();
};

class Sphere: public Object{
    private:
        float radius;

    public:
        Sphere(string name, Vector3 center, float radius, RGB ambientColor, RGB diffuseColor, RGB specularColor);
        Sphere(Vector3 center, float radius, RGB ambientColor, RGB diffuseColor, RGB specularColor);

        Vector3 getAABBminCoords();
        Vector3 getAABBmaxCoords();

        Vector3 getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay);
        RaycastHit getRaycastHit(Ray ray);
};

class Surface: public Object{
    private:
        Vector3 normal;

    public:
        Surface(string name, Vector3 position, Vector3 normal, RGB ambientColor, RGB diffuseColor, RGB specularColor);
        Surface(Vector3 position, Vector3 normal, RGB ambientColor, RGB diffuseColor, RGB specularColor);

        Vector3 getAABBminCoords();
        Vector3 getAABBmaxCoords();

        Vector3 getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay);
        RaycastHit getRaycastHit(Ray ray);
};

class Cylinder: public Object{
    private:
        Vector3 axis;
        float height;
        float radius;
        
        bool closed; //Indica se o cilindro tem as tampas ou não

        RaycastHit getSurfaceHit(Ray ray);
        RaycastHit getBottomCircleHit(Ray ray);
        RaycastHit getTopCircleHit(Ray ray);
    public:
        Cylinder(string name, Vector3 position, Vector3 axis, float ray, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed);
        Cylinder(Vector3 position, Vector3 axis, float ray, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed);
        Cylinder(Vector3 position, Vector3 axis, float ray, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor);

        Vector3 getAABBminCoords();
        Vector3 getAABBmaxCoords();

        Vector3 getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay);
        RaycastHit getRaycastHit(Ray ray);
};

class Cone: public Object{
    private:
        Vector3 axis;
        float height;
        float radius;

        bool closed; //Indica se o cone tem a base circular ou não

        RaycastHit getSurfaceHit(Ray ray);
        RaycastHit getBottomCircleHit(Ray ray);
    public:
        Cone(string name, Vector3 position, Vector3 axis, float ray, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed);
        Cone(Vector3 position, Vector3 axis, float ray, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed);
        Cone(Vector3 position, Vector3 axis, float ray, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor);

        Vector3 getAABBminCoords();
        Vector3 getAABBmaxCoords();

        Vector3 getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay);
        RaycastHit getRaycastHit(Ray ray);
};

class Triangle: public Object{
    private:
        Mesh* mesh = nullptr; //Referência à malha que o contém. nullptr se não fizer parte de nenhuma!
        int id = -1; //Id do triângulo relativo à malha que o contém. -1 se não pertencer a nenhuma malha

        static RGB getColorAtTexture(Triangle& t, Vector3 baryCoords);

    public:
        static Vector3 getMirroredPointOverPlane(Vector3 point, Vector3 plane_pos, Vector3 plane_normal);
        static Vector3 getPointRotationAroundArbitraryAxis(float degrees, Vector3 point, Vector3 rotCenter, Vector3 axis);

        Vector3 v1;
        Vector3 v2;
        Vector3 v3;

        UV uv1;
        UV uv2;
        UV uv3;

        Texture* tex = nullptr; //nullptr se não tiver nenhuma textura relacionada a essa face

        Vector3 standard_normal;
        Vector3 centroid;

        Triangle();
        Triangle(string name, Vector3 v1, Vector3 v2, Vector3 v3, RGB ambientColor, RGB diffuseColor, RGB specularColor);
        Triangle(Vector3 v1, Vector3 v2, Vector3 v3, RGB ambientColor, RGB diffuseColor, RGB specularColor);

        Mesh* getMesh();

        void move(Vector3 dir) override;

        Vector3 getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay);
        RaycastHit getRaycastHit(Ray ray);

        void scale(Vector3 scaleFactors, Vector3 pivot);

        void translate(Vector3 dir);

        void rotateX(float degrees, Vector3 rotCenter);
        void rotateY(float degrees, Vector3 rotCenter);
        void rotateZ(float degrees, Vector3 rotCenter);
        void rotateAroundAxis(float degrees, Vector3 rotCenter, Vector3 axis);

        void mirrorOverPlane(Vector3 plane_pos, Vector3 plane_normal);

        void shearX(float sz, float sy, Vector3 pivot);
        void shearY(float sx, float sz, Vector3 pivot);
        void shearZ(float sy, float sx, Vector3 pivot);

        Vector3 getAABBminCoords();
        Vector3 getAABBmaxCoords();

        friend class Mesh;
};

class Mesh: public Object{
    private:
        Triangle* triangles;

        Vector3 pivot;
        Vector3 pivot_x;
        Vector3 pivot_y;
        Vector3 pivot_z;

        int facesNumber;

        void computePivot();
    public:
        Mesh(string name, Vertex* vertices, Face* faces, UV* uv_coords, int facesNumber);
        Mesh(Vertex* vertices, Face* faces, UV* uv_coords, int facesNumber);
        ~Mesh();

        static Mesh* FromObj(const string& filename);

        int getFacesCount();
        Triangle* getFaces();
        Vector3 getPosition();
        Vector3 getPivot();

        void changeColor(RGB newColor);

        Vector3 getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay);
        RaycastHit getRaycastHit(Ray ray);

        void scale(Vector3 scalingFactors);

        void translate(Vector3 dir);

        void rotateX(float degrees);
        void rotateY(float degrees);
        void rotateZ(float degrees);

        void mirrorX();
        void mirrorY();
        void mirrorZ();

        void shearX(float sz, float sy);
        void shearY(float sx, float sz);
        void shearZ(float sy, float sx);

        void move(Vector3 dir); //Mesma coisa que o translate, mas esse é um override do move da classe Object
        void setPosition(Vector3 pos);

        Vector3 getAABBminCoords();
        Vector3 getAABBmaxCoords();

        friend class Scene;
};

struct AABB{
    Vector3 min;
    Vector3 max;

    static AABB build(Object& t);
    static AABB combine(std::vector<int> &indices, std::vector<AABB> &boxes);
    static bool rayIntersectsAABB(Ray& ray, const AABB& box);
};

struct BVHNode{
    AABB bbox;
    BVHNode* left;
    BVHNode* right;
    Object* obj; //Apenas para folhas (nullptr, caso não seja folha)

    static void __split(std::vector<int>& indices, std::vector<int>& leftIndices, std::vector<int>& rightIndices, std::vector<AABB>& boxes, float splitValue, int axis);
    static BVHNode* __rec_build(std::vector<Object*>& objs, std::vector<int>& indices, std::vector<AABB>& boxes);
    static BVHNode* build(std::vector<Object*>& objs, int objCount);
    static RaycastHit rayIntersectsBVH(Ray& ray, BVHNode* root);
};

#endif