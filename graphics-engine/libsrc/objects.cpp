#include <iostream>
#include <vector>
#include <math.h>
#include <omp.h>

#include <chrono>

#include "../../sdl/sdl/include/SDL.h"
#include "../../glew/glew/include/GL/glew.h"

#include "../include/datatypes.h"
#include "../include/objects.h"
#include "../include/raycast.h"
#include "../include/obj_importer.h"

using namespace std;

//Util
int findMinHitIndex(RaycastHit* hits, int size){
    float minValue = INFINITY;
    int minIndex = -1;

    for (int i = 0; i < size; ++i) {

        if (hits[i].functionParameter > 0.0f && hits[i].functionParameter < minValue) {
            minValue = hits[i].functionParameter;
            minIndex = i;
        }
    }
    
    return minIndex;
}

float positiveMin(float n1, float n2){ //Se retornar um número negativo, não existe um mínimo negativo entre os dois números
    if (n1 < 0.0f) return n2;
    if (n2 < 0.0f) return n1;

    return std::min(n1, n2);
}

Vector3 minCoordsOfTriangle(Triangle& t){
    Vector3& v1 = t.v1;
    Vector3& v2 = t.v2;
    Vector3& v3 = t.v3;
    
    float x_min, y_min, z_min;
    x_min = fminf(v1.getX(), v2.getX()); x_min = fminf(x_min, v3.getX());
    y_min = fminf(v1.getY(), v2.getY()); y_min = fminf(y_min, v3.getY());
    z_min = fminf(v1.getZ(), v2.getZ()); z_min = fminf(z_min, v3.getZ());

    return Vector3(x_min, y_min, z_min);
}

Vector3 maxCoordsOfTriangle(Triangle& t) {
    Vector3& v1 = t.v1;
    Vector3& v2 = t.v2;
    Vector3& v3 = t.v3;

    float x_max, y_max, z_max;
    x_max = fmaxf(v1.getX(), v2.getX()); x_max = fmaxf(x_max, v3.getX());
    y_max = fmaxf(v1.getY(), v2.getY()); y_max = fmaxf(y_max, v3.getY());
    z_max = fmaxf(v1.getZ(), v2.getZ()); z_max = fmaxf(z_max, v3.getZ());

    return Vector3(x_max, y_max, z_max);
}

void freeBVHNode(BVHNode* node) {
    if (!node) return;

    freeBVHNode(node->left);
    freeBVHNode(node->right);

    delete node;
}

//Classe Object
Object::Object() {}

Object::Object(string name, Vector3 position, RGB ambientColor, RGB diffuseColor, RGB specularColor, ObjType type) {
    this->type = type;

    this->name = (char*) name.c_str();
    this->position = position;

    this->ambientColor = ambientColor;
    this->diffuseColor = diffuseColor;
    this->specularColor = specularColor;
    this->specularExponent = 17.0f;
}

void Object::move(Vector3 dir){
    this->position = this->position + dir;

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

RGB Object::getAmbientColor(){
    return ambientColor;
}

RGB Object::getDiffuseColor() { 
    return diffuseColor; 
}

RGB Object::getSpecularColor() { 
    return specularColor; 
}

float Object::getSpecularExponent(){
    return specularExponent;
}

Vector3 Object::getPosition() {return this->position;}

void Object::setSceneOwnership(Scene* s) {this->s_owner = s;}

void Object::setName(string newName) {this->name = newName;}
string Object::getName() {return this->name;} 

//Classe Frame
Frame::Frame(float width, float height, int columns, int rows){
    this->default_width = width;
    this->default_height = height;

    this->width = width;
    this->height = height;
    this->columns = columns;
    this->rows = rows;

    this->position = Vector3(0,0,0);

    this->dX = this->width/this->columns;
    this->dY = this->height/this->rows;
}
Frame::Frame(): Frame(1,1,1,1){}

void Frame::move(Vector3 dir){
    position = position + dir;
}

void Frame::moveTo(Vector3 newPos){
    position = newPos;
}

void Frame::rotateXZ(float degrees, float dFrame){
    float rads = degrees * M_PI/180;
    float cosAng = cos(rads);
    float sinAng = sin(rads);

    float x = FRONT_DIR.cartesianDot(RIGHT_DIR);
    float y = FRONT_DIR.cartesianDot(UP_DIR) * cosAng + sinAng;
    float z = cosAng + FRONT_DIR.cartesianDot(UP_DIR) * -sinAng;

    FRONT_DIR = (RIGHT_DIR * x + UP_DIR * y + FRONT_DIR * z).normalize();
    UP_DIR = RIGHT_DIR.cartesianCross(FRONT_DIR).normalize();
}

void Frame::rotateY(float degrees, float dFrame){
    float rads = degrees * M_PI/180;
    float cosAng = cos(rads);
    float sinAng = sin(rads);

    FRONT_DIR = Vector3(
        FRONT_DIR.getX() * cosAng + FRONT_DIR.getZ() * sinAng,
        FRONT_DIR.getY(),
        FRONT_DIR.getX() * -sinAng + FRONT_DIR.getZ() * cosAng
    ).normalize();

    UP_DIR = Vector3(
        UP_DIR.getX() * cosAng + UP_DIR.getZ() * sinAng,
        UP_DIR.getY(),
        UP_DIR.getX() * -sinAng + UP_DIR.getZ() * cosAng
    ).normalize();

    RIGHT_DIR = FRONT_DIR.cartesianCross(UP_DIR).normalize();
}

float Frame::getDefaultWidth() {return default_width;}
float Frame::getDefaultHeight() {return default_height;}

float Frame::getWidth() {return width;}
float Frame::getHeight() {return height;}

void Frame::setWidth(float width) {this->width = width;}
void Frame::setHeight(float height) {this->height = height;}

int Frame::getColumns() {return columns;}
int Frame::getRows() {return rows;}
Vector3 Frame::getPosition() {return position;}
Vector3 Frame::getCamPosition() {return camPosition;}

Vector3 Frame::getFrontDir() {return FRONT_DIR;}
Vector3 Frame::getRightDir() {return RIGHT_DIR;}
Vector3 Frame::getUpDir() {return UP_DIR;}

Vector3 Frame::getPixelPos(int row, int col){
    Vector3 upperLeft = position + (UP_DIR * height)/2.0f - (RIGHT_DIR * width)/2.0f;
    Vector3 pixelPos = upperLeft + RIGHT_DIR * (dX/2.0f + dX * col) - UP_DIR * (dY/2.0f + dY * row); 

    return pixelPos;
}

//Classe Camera
Camera::Camera(Vector3 eyePos, Frame frame, float dFrame){
    this->eyePos = eyePos;
    this->frame = frame;
    this->dFrame = dFrame;

    this->orthogonal_frame_scale = 1.0f;

    this->oblique_frame_scale = 1.0f;
    this->oblique_inclination_in_degrees = 30.0f;

    this->frame.position = this->eyePos + this->frame.FRONT_DIR * this->dFrame;
    this->frame.camPosition = this->frame.FRONT_DIR * this->dFrame;
}
Camera::Camera() : Camera::Camera(Vector3(0,0,0), Frame(1,1,1,1), 30.0f) {}

Vector3 Camera::getEyePos(){return eyePos;}
Frame& Camera::getFrame(){return frame;}

void Camera::move(Vector3 dir){
    this->eyePos = this->eyePos + dir;
    this->frame.move(dir);
}

void Camera::LookAt(Vector3 pos){
    Frame& f = this->getFrame();

    f.FRONT_DIR = (pos - this->eyePos).normalize();
    f.RIGHT_DIR = f.FRONT_DIR.cartesianCross(Vector3(0, 1.0f, 0)).normalize();
    f.UP_DIR = f.RIGHT_DIR.cartesianCross(f.FRONT_DIR).normalize();

    f.position = this->eyePos + this->frame.FRONT_DIR * this->dFrame;
}

void Camera::rotateXZ(float degrees){
    this->frame.rotateXZ(degrees, dFrame);

    this->frame.position = this->eyePos + this->frame.FRONT_DIR * this->dFrame;
}
void Camera::rotateY(float degrees){
    this->frame.rotateY(degrees, dFrame);

    this->frame.position = this->eyePos + this->frame.FRONT_DIR * this->dFrame;
}

void Camera::changeVerticalFov(float degrees){
    double fov_tg = std::tan(degrees/2.0f * M_PI/180.0f);
    this->dFrame = frame.height/(2.0f * fov_tg);

    this->frame.position = this->eyePos + this->frame.FRONT_DIR * this->dFrame;
    this->frame.camPosition = this->frame.FRONT_DIR * this->dFrame;
}

Vector3 Camera::getFrameCameraPosition() {return (this->getFrame().FRONT_DIR * this->dFrame);}

//Classe Scene
Scene::Scene(Camera camera, SDL_Window* canvas){
    this->camera = camera;
    this->canvas = canvas;
}

Scene::Scene(Camera camera){
    this->camera = camera;
}

void Scene::setCanvas(SDL_Window* canvas){
    this->canvas = canvas;
}

void Scene::addObject(Object* o) {
    objects.push_back(o);

    if (o->type == ObjType::TMesh){
        Mesh* m = dynamic_cast<Mesh*>(o);
        Triangle* triangles = m->getFaces();

        for (int i = 0; i < m->getFacesCount(); ++i){
            plain_objects.push_back(triangles + i);
        }
    }
    else{
        plain_objects.push_back(o);
    }

    o->setSceneOwnership(this);

    cacheBVH();
}

void Scene::addLight(Light* l){
    lightSources.push_back(l);
}

void Scene::removeObject(Object* obj){
    //Deleção do objeto no vetor plain_objects
    if (obj->type == ObjType::TMesh){
        Mesh* m = dynamic_cast<Mesh*>(obj);

        for (int i = 0; i < this->plain_objects.size();){ //Deletar todos os triângulos dessa malha no vetor plain_objects
            if (this->plain_objects[i]->type == ObjType::TTriangle){
                Triangle* t = dynamic_cast<Triangle*>(this->plain_objects[i]);

                if (t->getMesh() == m){
                    this->plain_objects.erase(this->plain_objects.begin() + i);
                    continue; //Os elementos sofrem shift para esquerda!!!
                }
            }

            ++i;
        }
    }
    else{
        for (int i = 0; i < this->plain_objects.size(); ++i){
            if (this->plain_objects[i] == obj){
                this->plain_objects.erase(this->plain_objects.begin() + i);
                break;
            }
        }
    }

    //Deleção do objeto no vetor objects
    for (int i = 0; i < this->objects.size(); ++i){
        if (this->objects[i] == obj){
            this->objects.erase(this->objects.begin() + i);
            break;
        }
    }

    if (obj->type == ObjType::TMesh){
        Mesh* m = dynamic_cast<Mesh*>(obj);
        
        delete m;
    }
    else delete obj;

    this->cacheBVH(); //Reconstrói a bvh com o novo conjunto de objetos
}

void Scene::cacheBVH(){
    this->cachedBVH = BVHNode::build(this->plain_objects, this->plain_objects.size());
}

void Scene::requestBVHCaching(){
    this->cacheBVH();
}

std::vector<Object*> Scene::getObjects() { 
    return objects; 
}

std::vector<Light*> Scene::getLightSources(){
    return lightSources;
}

Camera& Scene::getCamera(){
    return camera;
}

Object* Scene::pick(int x, int y){
    Vector3 eyePos = this->getCamera().getEyePos();
    Vector3 pixelPos = this->getCamera().getFrame().getPixelPos(y, x);

    Ray ray = Ray(eyePos, (pixelPos - eyePos).normalize());

    RaycastHit hit = this->Raycast(ray);
    if (hit.hit){
        Object* obj = hit.object;

        if (obj->type == ObjType::TTriangle){
            Triangle* t = dynamic_cast<Triangle*>(obj);

            if (t->getMesh() != nullptr) return t->getMesh();
            else return t;  
        }
        else return obj;
    }
    else return nullptr;
}

SDL_Window* Scene::getCanvas() {return this->canvas;}

RaycastHit Scene::Raycast(Ray rayInfo) {
    float smallestParameter = INFINITY;
    RaycastHit hit = BVHNode::rayIntersectsBVH(rayInfo, this->cachedBVH);
    
    return hit;
}

RGB Scene::computeShading(RaycastHit hit, RGB AMBIENT_ENERGY){
    RGB sum = RGB(0,0,0);

    for (Light* l: this->lightSources){
        if (!l->isOn()) continue;
        sum = (Vector3) sum + l->evaluateShading(*this, hit);
    }

    RGB ambientColor = hit.object->getAmbientColor();
    if (hit.hasTexture) ambientColor = hit.textureColor;

    sum = (Vector3) AMBIENT_ENERGY * ambientColor + sum;

    return sum;
}

void Scene::drawScene(){
    auto start = std::chrono::high_resolution_clock::now();

    Frame& f = this->camera.getFrame();
    Vector3 eyePos = this->camera.getEyePos(); 

    float dX = f.getWidth()/f.getColumns();
    float dY = f.getHeight()/f.getRows();

    if (0 == 1){
        glBegin(GL_POINTS); //Começar a desenhar no back buffer
        for (int r = 0; r <= f.getRows(); ++r){  
            float frame_pixelY = f.getPosition().getY() + f.getHeight()/2 - dY/2 - r*dY;   

            for (int c = 0; c <= f.getColumns(); ++c){ 
                float frame_pixelX = f.getPosition().getX() - f.getWidth()/2 + dX/2 + c*dX;

                Vector3 frame_pixelCoord = Vector3(frame_pixelX, frame_pixelY, f.getPosition().getZ());

                //rayDir depende da projeção
                Vector3 rayDir;
                if (Scene::camProjection == CameraProjection::Perspective){
                    rayDir = (frame_pixelCoord - eyePos).normalize();
                }
                else if (Scene::camProjection == CameraProjection::Orthographic){
                    rayDir = f.getFrontDir(); //Direção do eixo frontal da camera (paralelo aos pixels)
                }

                RaycastHit hitResult = this->Raycast(Ray(eyePos, rayDir));//s.getHit(eyePos, rayDir);         

                if (!hitResult.hit){ //Se não houver interseção...
                    glColor3f(135.0f/255.0f, 206.0f/255.0f, 235.0f/255.0f);
                    glVertex2f(-1.0f + (2.0f*c)/(f.getColumns()), 1.0f - (2.0f*(r))/(f.getRows()));
                }
                else{
                    RGB shading = this->computeShading(hitResult, RGB(0.3f, 0.3f, 0.3f));
                    
                    glColor3f(shading.getR(), shading.getG(), shading.getB());
                    glVertex2f(-1.0f + (2.0f*c)/(f.getColumns()), 1.0f - (2.0f*(r))/(f.getRows()));
                }
            }
        }
        glEnd(); //Terminar de desenhar no back buffer
    }
    
    //Para tirar paralelo exclui daq
    std::vector<std::vector<RGB>> colorBuffer(f.getRows() + 1, std::vector<RGB>(f.getColumns() + 1, RGB(0.0f, 0.0f, 0.0f)));

    omp_set_num_threads(36);
    #pragma omp parallel for collapse(2)
    for (int r = 0; r < f.getRows(); ++r) {
        for (int c = 0; c < f.getColumns(); ++c) {
            if (Scene::optimizedRenderization){
                if (r % this->optimizationLevel != 0) continue;
                if (c % this->optimizationLevel != 0) continue;
            }

            Vector3 frame_pixelCoord = f.getPixelPos(r, c);
            
            Vector3 rayDir;
            Vector3 rayStartPos;
            if (Scene::camProjection == CameraProjection::Perspective){
                rayDir = (frame_pixelCoord - eyePos).normalize();
                rayStartPos = eyePos;
            }
            else if (Scene::camProjection == CameraProjection::Orthographic){
                rayDir = f.getFrontDir(); //Direção do eixo frontal da camera (paralelo aos pixels)
                rayStartPos = frame_pixelCoord - f.getFrontDir() * this->camera.dFrame; //Raio vai partir do olho para maior coerência com a projeção perspectiva
            }
            else if (Scene::camProjection == CameraProjection::Oblique){
                //Se aproveitando dessa função da classe Triangle ( talvez eu deveria ter botado ela em outro lugar ): )
                rayDir = Triangle::getPointRotationAroundArbitraryAxis(-this->getCamera().oblique_inclination_in_degrees, f.getFrontDir(), Vector3(0,0,0), f.getRightDir()).normalize(); //Normalizando de novo para minimizar erros
                rayStartPos = frame_pixelCoord - f.getFrontDir() * this->camera.dFrame; //Partindo do olho também!
            }

            RaycastHit hitResult = this->Raycast(Ray(rayStartPos, rayDir));        

            if (!hitResult.hit){

                if (Scene::mode == RenderizationMode::Wireframe){
                    colorBuffer[r][c] = RGB(0.0f, 0.0f, 0.0f);
                }
                else colorBuffer[r][c] = RGB(135.0f/255.0f, 206.0f/255.0f, 235.0f/255.0f);

            } else {

                if (Scene::mode == RenderizationMode::Wireframe){
                    colorBuffer[r][c] = RGB(0.0f, 1.0f, 0.0f);
                }
                else colorBuffer[r][c] = this->computeShading(hitResult, RGB(0.25f, 0.25f, 0.25f)); // Sombreamento
                
            }
        }
    }
    
    if (Scene::optimizedRenderization && Scene::should_predict_pixel){
        for (int r = 0; r < f.getRows(); ++r) {
            for (int c = 0; c < f.getColumns(); ++c){
                if (r % this->optimizationLevel != 0){
                    colorBuffer[r][c] = colorBuffer[r - 1][c];
                }
                if (c % this->optimizationLevel != 0){
                    colorBuffer[r][c] = colorBuffer[r][c - 1];
                }
            }
        }
    }

    // Fase 2: Renderização com OpenGL (não paralelizada)
    glBegin(GL_POINTS); 
    for (int r = 0; r < f.getRows(); ++r) {
        for (int c = 0; c < f.getColumns(); ++c) {
            RGB shading = colorBuffer[r][c];
            glColor3f(shading.getR(), shading.getG(), shading.getB());
            glVertex2f(-1.0f + (2.0f * c) / f.getColumns(), 1.0f - (2.0f * (r+1)) / f.getRows());
        }
    }
    //Até aqui
    glEnd();

    //SDL_GL_SwapWindow(this->canvas); //Transferir o back buffer para o front buffer

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;
    //std::cout << "A cena renderizou em: " << duration.count() << " ms" << std::endl;
}
void Scene::renderScene(){
    drawScene();
}

int Scene::optimizationLevel = 5;
bool Scene::optimizedRenderization = false;
void Scene::switchOptimizedRenderization(){
    if (Scene::optimizedRenderization) Scene::optimizedRenderization = false;
    else Scene::optimizedRenderization = true;
}
bool Scene::should_predict_pixel = false;

RenderizationMode Scene::mode = RenderizationMode::Default;
void Scene::changeRenderizationMode(RenderizationMode mode){
    Scene::mode = mode;
}

TextureFiltering Scene::texFiltering = TextureFiltering::Nearest;
void Scene::changeTextureFiltering(TextureFiltering filter){
    Scene::texFiltering = filter;
}

CameraProjection Scene::camProjection = CameraProjection::Perspective;
void Scene::changeCameraProjection(CameraProjection projection){
    Scene::camProjection = projection;
}

void Scene::updateFrameProportions(){
    Frame& f = this->getCamera().getFrame();

    if (Scene::camProjection == CameraProjection::Perspective){
        f.setWidth(f.getDefaultWidth());
        f.setHeight(f.getDefaultHeight());
    }
    else if (Scene::camProjection == CameraProjection::Orthographic){
        f.setWidth(f.getDefaultWidth() * this->getCamera().orthogonal_frame_scale);
        f.setHeight(f.getDefaultHeight() * this->getCamera().orthogonal_frame_scale);
    }
    else if (Scene::camProjection == CameraProjection::Oblique){
        f.setWidth(f.getDefaultWidth() * this->getCamera().oblique_frame_scale);
        f.setHeight(f.getDefaultHeight() * this->getCamera().oblique_frame_scale);
    }
}

//Classe Sphere
Sphere::Sphere(string name, Vector3 center, float radius, RGB ambientColor, RGB diffuseColor, RGB specularColor) 
    : Object(name, center, ambientColor, diffuseColor, specularColor, ObjType::TSphere) {this->radius = radius;}

Sphere::Sphere(Vector3 center, float radius, RGB ambientColor, RGB diffuseColor, RGB specularColor) 
    : Sphere("Sphere", center, radius, ambientColor, diffuseColor, specularColor) {}

Vector3 Sphere::getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay) {
    //Supomos que o ponto está exatamente na esfera (ignorando as aproximações do ponto flutuante)
    //Com isso, vamos usar um fator de correção para saber se o raio partiu de dentro ou de fora da esfera

    Vector3 notNormalizedNormal = ((intersectionPoint - intersectionRay.getDirection() * 0.001f) - this->position); //0.001f é o fator de correção

    if (notNormalizedNormal.magnitude() < this->radius) notNormalizedNormal = -notNormalizedNormal;

    return notNormalizedNormal.normalize();
}

RaycastHit Sphere::getRaycastHit(Ray ray) {
    Vector3 v = ray.getInitialPos() - this->position;
    Vector3 dir = ray.getDirection();

    float a = dir.cartesianDot(dir);
    float b = 2 * dir.cartesianDot(v);
    float c = v.cartesianDot(v) - pow(this->radius, 2);

    float delta = pow(b, 2) - 4 * a * c;
    RaycastHit result;

    if (delta == 0.0f) {
        float t = -b / (2 * a) - 0.001f; // 0.001f vai ser o fator de correção para a nossa esfera

        result.hit = (t >= 0);
        result.object = this;
        result.functionParameter = t;
        result.hitPoint = ray.getInitialPos() + dir * t;
        result.normal = this->getNormalAtIntersection(result.hitPoint, ray);
    } else if (delta > 0.0f) {
        float t1 = (-b + sqrt(delta)) / (2 * a);
        float t2 = (-b - sqrt(delta)) / (2 * a);
        float minT = positiveMin(t1, t2) - 0.001f;

        result.hit = (minT >= 0);
        result.object = this;
        result.functionParameter = minT;
        result.hitPoint = ray.getInitialPos() + dir * minT;
        result.normal = this->getNormalAtIntersection(result.hitPoint, ray);
    }

    return result;
}

Vector3 Sphere::getAABBminCoords(){
    return Vector3(position.getX() - radius,
                   position.getY() - radius,
                   position.getZ() - radius);
}
Vector3 Sphere::getAABBmaxCoords(){
        return Vector3(position.getX() + radius,
                       position.getY() + radius,
                       position.getZ() + radius);
}

//Classe Surface
Surface::Surface(string name, Vector3 position, Vector3 normal, RGB ambientColor, RGB diffuseColor, RGB specularColor)
    : Object(name, position, ambientColor, diffuseColor, specularColor, ObjType::TSurface) {this->normal = normal.normalize();}
Surface::Surface(Vector3 position, Vector3 normal, RGB ambientColor, RGB diffuseColor, RGB specularColor)
    :  Surface("Surface", position, normal.normalize(), ambientColor, diffuseColor, specularColor) {}

Vector3 Surface::getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay) {
    //Vamos considerar que o ponto de intersecção está exatamente na superfície (ignorando as aproximações do ponto flutuante)
    //Com isso, vamos considerar que esse ponto é o centro do eixo direcionado na normal da superfície
    //Vamos usar um ponto corrigido pelo raio de intersecção para saber se esse ponto está na direção positiva ou negativa desse eixo!

    Vector3 correctedPoint = (intersectionPoint - intersectionRay.getDirection() * 0.001f) - intersectionPoint; //Esse ponto está corrigido e está no sistema de coodernadas centralizado no ponto de intersecção 

    if (correctedPoint.cartesianDot(this->normal) < 0.0f) return -(this->normal);
    else return this->normal;
}

RaycastHit Surface::getRaycastHit(Ray ray){
    RaycastHit hit = RaycastHit();

    Vector3 v = ray.getInitialPos() - this->position;

    float denominator = this->normal.cartesianDot(ray.getDirection());
    if (denominator == 0.0f) return hit; //Se for diferente de 0, prosseguimos com o cálculo de interseção

    float numerator = (-this->normal).cartesianDot(v);
    float t = numerator / denominator;

    if (t < 0.0f) return hit; //Queremos interseções que aconteçam na direção positiva do raio!

    t = t - 0.001f; //0.001f é o nosso fator de correção para a superfície

    hit.hit = true;
    hit.hitPoint = ray.getInitialPos() + ray.getDirection() * t;
    hit.normal = this->getNormalAtIntersection(hit.hitPoint, ray);
    hit.functionParameter = t;
    hit.object = this;

    return hit;
}

Vector3 Surface::getAABBminCoords() {return Vector3();}
Vector3 Surface::getAABBmaxCoords() {return Vector3();}

//Classe Cylinder
Cylinder::Cylinder(string name, Vector3 position, Vector3 axis, float radius, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed) 
    : Object(name, position, ambientColor, diffuseColor, specularColor, ObjType::TCylinder){ this->axis = axis.normalize(); this->radius = radius; this->height = height; this->closed = closed;}

Cylinder::Cylinder(Vector3 position, Vector3 axis, float radius, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed)
    : Cylinder("Cylinder", position, axis, radius, height, ambientColor, diffuseColor, specularColor, closed) {}

Cylinder::Cylinder(Vector3 position, Vector3 axis, float radius, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor)
    : Cylinder(position, axis, radius, height, ambientColor, diffuseColor, specularColor, true) {}

RaycastHit Cylinder::getBottomCircleHit(Ray ray){
    RaycastHit hit = RaycastHit();

    Vector3 base = this->position;

    float a = this->axis.cartesianDot(base - ray.getInitialPos());
    float b = this->axis.cartesianDot(ray.getDirection());

    if (b == 0.0f) return hit; //Paralelo à superfície

    float t = a/b;

    if (t < 0.0f) return hit;

    t -= 0.001f;

    Vector3 hitPoint = ray.getInitialPos() + ray.getDirection() * t;
    if ((base - hitPoint).magnitude() > this->radius) return hit; //O ponto tem que estar na área da circunferência da base!

    hit.hit = true;
    hit.hitPoint = hitPoint;
    hit.functionParameter = t;

    Vector3 normal = -this->axis;
    if (ray.getDirection().cartesianDot(this->axis) < 0.0f) normal = this->axis;

    hit.normal = normal;

    hit.object = this;

    return hit;
}

RaycastHit Cylinder::getTopCircleHit(Ray ray){
    RaycastHit hit = RaycastHit();

    Vector3 base = this->position + this->axis * this->height;

    float a = this->axis.cartesianDot(base - ray.getInitialPos());
    float b = this->axis.cartesianDot(ray.getDirection());

    if (b == 0.0f) return hit; //Paralelo à superfície

    float t = a/b;

    if (t < 0.0f) return hit;

    t -= 0.001f;

    Vector3 hitPoint = ray.getInitialPos() + ray.getDirection() * t;
    if ((base - hitPoint).magnitude() > this->radius) return hit; //O ponto tem que estar na área da circunferência da base!

    hit.hit = true;
    hit.hitPoint = hitPoint;
    hit.functionParameter = t;

    Vector3 normal = this->axis;
    if (ray.getDirection().cartesianDot(this->axis) > 0.0f) normal = -this->axis;

    hit.normal = normal;

    hit.object = this;

    return hit;
}

RaycastHit Cylinder::getSurfaceHit(Ray ray){
    RaycastHit hit = RaycastHit();

    Vector3 d = ray.getDirection();
    
    Vector3 X = ray.getInitialPos() - this->position - this->axis * (this->axis.cartesianDot(ray.getInitialPos() - this->position));
    Vector3 Y = ray.getDirection() - this->axis * (this->axis.cartesianDot(ray.getDirection()));

    float a = Y.cartesianDot(Y);
    float b = 2*(Y.cartesianDot(X));
    float c = X.cartesianDot(X) - pow(this->radius, 2);

    float delta = pow(b,2) - 4*a*c;

    float t1 = (-b + sqrt(delta))/(2*a);
    float t2 = (-b - sqrt(delta))/(2*a);

    if (a == 0.0f) return hit; //Raio é paralelo ao eixo do cilindro, deixa essa intersecção para as tampas do cilindro

    MinimumPositive minParameter = MinimumPositive();

    //Checando se t1 é válido segundo o critério da altura
    Vector3 hitPoint1 = ray.getInitialPos() + ray.getDirection() * t1;
    float proj1Size = (hitPoint1 - position).cartesianDot(axis);

    if (proj1Size >= 0 && proj1Size <= this->height) {minParameter.insert(t1);};

    //Checando se t2 é válido segundo o critério da altura
    Vector3 hitPoint2 = ray.getInitialPos() + ray.getDirection() * t2;
    float proj2Size = (hitPoint2 - position).cartesianDot(axis);

    if (proj2Size >= 0 && proj2Size <= this->height) minParameter.insert(t2);

    if (!minParameter.hasPositive()) return hit; //Nenhum parâmetro t foi positivo

    float parameter = minParameter.get() - 0.005f;

    hit.hit = true;
    hit.hitPoint = ray.getInitialPos() + ray.getDirection() * parameter;

    Vector3 normal = (hit.hitPoint - position - axis*((hit.hitPoint - position).cartesianDot(axis))).normalize();
    Vector3 proj = axis*(hit.hitPoint - position).cartesianDot(axis);
    float distToAxis = (hit.hitPoint - position - proj).magnitude(); //Distância corrigida, já que o parâmetro do raio está corrigido

    if (distToAxis < radius) normal = -normal;

    hit.normal = normal;

    hit.functionParameter = parameter;
    hit.object = this;

    return hit;
}

RaycastHit Cylinder::getRaycastHit(Ray ray){
    RaycastHit validHit = RaycastHit();
    validHit.functionParameter = INFINITY;

    RaycastHit surfaceHit = getSurfaceHit(ray);

    if (surfaceHit.hit && surfaceHit.functionParameter < validHit.functionParameter) validHit = surfaceHit;

    if (closed){
        RaycastHit bottomHit = getBottomCircleHit(ray);
        if (bottomHit.hit && bottomHit.functionParameter < validHit.functionParameter) validHit = bottomHit;

        RaycastHit topHit = getTopCircleHit(ray);
        if (topHit.hit && topHit.functionParameter < validHit.functionParameter) validHit = topHit;
    }

    return validHit; 
}

Vector3 Cylinder::getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay) {return Vector3();}

Vector3 Cylinder::getAABBminCoords() {
    Vector3 top = position + (axis * height);

    Vector3 right = Vector3(1, 0, 0) - axis * axis.getX();
    Vector3 up    = Vector3(0, 1, 0) - axis * axis.getY();
    Vector3 forward = Vector3(0, 0, 1) - axis * axis.getZ();

    right = right.normalize();
    up = up.normalize();
    forward = forward.normalize();

    Vector3 points[] = {
        position + right * radius,
        position - right * radius,
        position + forward * radius,
        position - forward * radius,
        top + right * radius,
        top - right * radius,
        top + forward * radius,
        top - forward * radius
    };

    float minX = points[0].getX();
    float minY = points[0].getY();
    float minZ = points[0].getZ();

    for (int i = 1; i < 8; i++) {
        minX = std::min(minX, points[i].getX());
        minY = std::min(minY, points[i].getY());
        minZ = std::min(minZ, points[i].getZ());
    }

    return Vector3(minX, minY, minZ);
}
Vector3 Cylinder::getAABBmaxCoords() {
    Vector3 top = position + (axis * height);

    // Compute the radius-aligned vectors
    Vector3 right = Vector3(1, 0, 0) - axis * axis.getX();
    Vector3 up    = Vector3(0, 1, 0) - axis * axis.getY();
    Vector3 forward = Vector3(0, 0, 1) - axis * axis.getZ();

    right = right.normalize();
    up = up.normalize();
    forward = forward.normalize();

    Vector3 points[] = {
        position + right * radius,
        position - right * radius,
        position + forward * radius,
        position - forward * radius,
        top + right * radius,
        top - right * radius,
        top + forward * radius,
        top - forward * radius
    };

    float maxX = points[0].getX();
    float maxY = points[0].getY();
    float maxZ = points[0].getZ();

    for (int i = 1; i < 8; i++) {
        maxX = std::max(maxX, points[i].getX());
        maxY = std::max(maxY, points[i].getY());
        maxZ = std::max(maxZ, points[i].getZ());
    }

    return Vector3(maxX, maxY, maxZ);
}

//Classe Cone
Cone::Cone(string name, Vector3 position, Vector3 axis, float radius, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed) 
    : Object(name, position, ambientColor, diffuseColor, specularColor, ObjType::TCone){ this->axis = axis.normalize(); this->radius = radius; this->height = height; this->closed = closed;}
Cone::Cone(Vector3 position, Vector3 axis, float radius, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor, bool closed) 
    : Cone("Cone", position, axis, radius, height, ambientColor, diffuseColor, specularColor, closed) {}
Cone::Cone(Vector3 position, Vector3 axis, float radius, float height, RGB ambientColor, RGB diffuseColor, RGB specularColor) 
    : Cone("Cone", position, axis, radius, height, ambientColor, diffuseColor, specularColor, true) {}

RaycastHit Cone::getBottomCircleHit(Ray ray){
    RaycastHit hit = RaycastHit();

    Vector3 base = this->position;

    float a = this->axis.cartesianDot(base - ray.getInitialPos());
    float b = this->axis.cartesianDot(ray.getDirection());

    if (b == 0.0f) return hit; //Paralelo à superfície

    float t = a/b;

    if (t < 0.0f) return hit;

    t -= 0.001f;

    Vector3 hitPoint = ray.getInitialPos() + ray.getDirection() * t;
    if ((base - hitPoint).magnitude() > this->radius) return hit; //O ponto tem que estar na área da circunferência da base!

    hit.hit = true;
    hit.hitPoint = hitPoint;
    hit.functionParameter = t;

    Vector3 normal = -this->axis;
    if (ray.getDirection().cartesianDot(this->axis) < 0.0f) normal = -this->axis;

    hit.normal = normal;

    hit.object = this;

    return hit;
}

RaycastHit Cone::getSurfaceHit(Ray ray){
    RaycastHit hit = RaycastHit();

    Vector3 W = -axis*(axis.cartesianDot(ray.getInitialPos())) + axis*(axis.cartesianDot(position));
    Vector3 X = axis*height + W;
    Vector3 Y = ray.getInitialPos() - position + W;
    Vector3 Z = -axis*(axis.cartesianDot(ray.getDirection()));
    Vector3 A = Z + ray.getDirection();

    float height2 = pow(height, 2.0f);
    float radius2 = pow(radius, 2.0f);

    float a = height2 * (A.cartesianDot(A)) - radius2 * (Z.cartesianDot(Z));
    float b = height2 * (2.0f*(Y.cartesianDot(A))) - radius2 * (2.0f*(X.cartesianDot(Z)));
    float c = height2 * (Y.cartesianDot(Y)) - radius2 * (X.cartesianDot(X));

    MinimumPositive minimumParameter = MinimumPositive();

    if (a == 0.0f){ //Raio é perpendicular a uma geratriz do cone! (intersecção se reduz a uma função do primeiro grau)
        float t = -c/b; if (b == 0.0f) return hit;
        
        //Testando se o ponto é válido
        Vector3 hitPoint = ray.getInitialPos() + ray.getDirection() * t;
        float projSize = (hitPoint - position).cartesianDot(axis);

        if (projSize >= 0.0f && projSize <= height) minimumParameter.insert(t);

        hit.hit = minimumParameter.hasPositive();
        hit.functionParameter = minimumParameter.get() - 0.005f;
        hit.hitPoint = ray.getInitialPos() + ray.getDirection() * hit.functionParameter;

        Vector3 s = hit.hitPoint - position;
        Vector3 base = s - axis*(axis.cartesianDot(s));
        Vector3 b_axis = base.normalize();
        Vector3 h_axis = this->axis;
        Vector3 gen = (base - axis*height).normalize();
    
        float x = (gen.cartesianDot(b_axis));
        float y = (gen.cartesianDot(h_axis));

        hit.normal = (b_axis*(-y) + h_axis*x);

        //Se o raio estiver na mesma direção da normal, então o raio atingiu um ponto interior ao cone!!!
        if (ray.getDirection().cartesianDot(hit.normal) > 0.0f) hit.normal = -hit.normal;

        hit.object = this;

        return hit;
    }

    float delta = pow(b,2.0f) - 4.0f*a*c;

    //Checando se o primeiro ponto é válido
    float t1 = (-b + sqrt(delta))/(2.0f*a);
    Vector3 hitPoint1 = ray.getInitialPos() + ray.getDirection() * t1;
    float proj1Size = (hitPoint1 - position).cartesianDot(axis);

    if (proj1Size >= 0.0f && proj1Size <= height) minimumParameter.insert(t1);

    //Checando se o segundo ponto é válido
    float t2 = (-b - sqrt(delta))/(2.0f*a);
    Vector3 hitPoint2 = ray.getInitialPos() + ray.getDirection() * t2;
    float proj2Size = (hitPoint2 - position).cartesianDot(axis);

    if (proj2Size >= 0.0f && proj2Size <= height) minimumParameter.insert(t2);

    hit.hit = minimumParameter.hasPositive();
    hit.functionParameter = minimumParameter.get() - 0.005f;

    hit.hitPoint = ray.getInitialPos() + ray.getDirection() * hit.functionParameter;

    //Vamos rotacionar a geratriz correspondente ao ponto de intersecção em 90 graus, isso equivale à normal desse ponto
    Vector3 s = hit.hitPoint - position;
    Vector3 base = s - axis*(axis.cartesianDot(s));
    Vector3 b_axis = base.normalize();
    Vector3 h_axis = this->axis;
    Vector3 gen = (base - axis*height).normalize();
  
    float x = (gen.cartesianDot(b_axis));
    float y = (gen.cartesianDot(h_axis));

    hit.normal = (b_axis*(-y) + h_axis*x);

    //Se o raio estiver na mesma direção da normal, então o raio atingiu um ponto interior ao cone!!!
    if (ray.getDirection().cartesianDot(hit.normal) > 0.0f) hit.normal = -hit.normal;

    hit.object = this;

    return hit;
}

RaycastHit Cone::getRaycastHit(Ray ray){
    RaycastHit validHit = RaycastHit();
    validHit.functionParameter = INFINITY;

    RaycastHit surfaceHit = getSurfaceHit(ray);

    if (surfaceHit.hit && surfaceHit.functionParameter < validHit.functionParameter) validHit = surfaceHit;

    if (closed){
        RaycastHit bottomHit = getBottomCircleHit(ray);
        if (bottomHit.hit && bottomHit.functionParameter < validHit.functionParameter) validHit = bottomHit;
    }

    return validHit; 
}

Vector3 Cone::getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay) {return Vector3();}

Vector3 Cone::getAABBminCoords() {
    Vector3 apex = position + (axis * height);

    Vector3 right = Vector3(1, 0, 0) - axis * axis.getX();
    Vector3 up    = Vector3(0, 1, 0) - axis * axis.getY();
    Vector3 forward = Vector3(0, 0, 1) - axis * axis.getZ();

    right = right.normalize();
    up = up.normalize();
    forward = forward.normalize();

    Vector3 p1 = position + right * radius;
    Vector3 p2 = position - right * radius;
    Vector3 p3 = position + forward * radius;
    Vector3 p4 = position - forward * radius;

    float minX = p1.getX();
    float minY = p1.getY();
    float minZ = p1.getZ();

    minX = std::min(minX, std::min(apex.getX(), std::min(p2.getX(), std::min(p3.getX(), p4.getX()))));
    minY = std::min(minY, std::min(apex.getY(), std::min(p2.getY(), std::min(p3.getY(), p4.getY()))));
    minZ = std::min(minZ, std::min(apex.getZ(), std::min(p2.getZ(), std::min(p3.getZ(), p4.getZ()))));

    return Vector3(minX, minY, minZ);
}
Vector3 Cone::getAABBmaxCoords() {
    Vector3 apex = position + (axis * height);

    Vector3 right = Vector3(1, 0, 0) - axis * axis.getX();
    Vector3 up    = Vector3(0, 1, 0) - axis * axis.getY();
    Vector3 forward = Vector3(0, 0, 1) - axis * axis.getZ();

    right = right.normalize();
    up = up.normalize();
    forward = forward.normalize();

    Vector3 p1 = position + right * radius;
    Vector3 p2 = position - right * radius;
    Vector3 p3 = position + forward * radius;
    Vector3 p4 = position - forward * radius;

    float maxX = p1.getX();
    float maxY = p1.getY();
    float maxZ = p1.getZ();

    maxX = std::max(maxX, std::max(apex.getX(), std::max(p2.getX(), std::max(p3.getX(), p4.getX()))));
    maxY = std::max(maxY, std::max(apex.getY(), std::max(p2.getY(), std::max(p3.getY(), p4.getY()))));
    maxZ = std::max(maxZ, std::max(apex.getZ(), std::max(p2.getZ(), std::max(p3.getZ(), p4.getZ()))));

    return Vector3(maxX, maxY, maxZ);
}

//Classe Triangle
Triangle::Triangle(string name, Vector3 v1, Vector3 v2, Vector3 v3, RGB ambientColor, RGB diffuseColor, RGB specularColor)
    : Object(name, Vector3(0,0,0), ambientColor, diffuseColor, specularColor, ObjType::TTriangle) {
        this->v1 = v1; this->v2 = v2; this->v3 = v3; 
        this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
        this->centroid = (v1 + v2 + v3)/3;
    }
Triangle::Triangle(Vector3 v1, Vector3 v2, Vector3 v3, RGB ambientColor, RGB diffuseColor, RGB specularColor)
    : Triangle("Triangle", v1, v2, v3, ambientColor, diffuseColor, specularColor) {}
Triangle::Triangle() {}

RaycastHit Triangle::getRaycastHit(Ray ray){
    //u = v2, v = v3, w = v1

    RaycastHit hit = RaycastHit();

    Vector3 edge1 = v2 - v1;
    Vector3 edge2 = v3 - v1;
    Vector3 pVec = ray.getDirection().cartesianCross(edge2);
    float det = edge1.cartesianDot(pVec);

    if (fabs(det) < 0.00001f) return hit; // No intersection (parallel)

    float invDet = 1.0f / det;
    Vector3 tVec = ray.getInitialPos() - v1;
    float u = tVec.cartesianDot(pVec) * invDet;
    if (u < 0.0f || u > 1.0f) return hit;

    Vector3 qVec = tVec.cartesianCross(edge1);
    float v = ray.getDirection().cartesianDot(qVec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return hit;

    float t = edge2.cartesianDot(qVec) * invDet;
    if (t < 0.0f) return hit;

    t = t - 0.01f;

    if (Scene::mode == RenderizationMode::Wireframe){
        const float edgeThreshold = 0.01f;
        bool nearEdge = (u < edgeThreshold || u > 1.0f - edgeThreshold) ||
                        (v < edgeThreshold || v > 1.0f - edgeThreshold) ||
                        (u + v < edgeThreshold || u + v > 1.0f - edgeThreshold);

        if (!nearEdge) return hit;
    }

    hit.hit = true;
    hit.hitPoint = ray.getInitialPos() + ray.getDirection() * t;
    hit.normal = this->getNormalAtIntersection(hit.hitPoint, ray);
    hit.functionParameter = t;
    hit.id = this->id;
    hit.object = this;

    hit.baryCoords = Vector3(1 - u - v, u, v);

    if (this->tex != nullptr){
        hit.hasTexture = true;
        hit.textureColor = Triangle::getColorAtTexture(*this, hit.baryCoords);
    }

    return hit;
}

Vector3 Triangle::getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay) {
    Vector3 intersectionOri = -intersectionRay.getDirection();

    if (intersectionOri.cartesianDot(this->standard_normal) > 0) return this->standard_normal;
    else return -this->standard_normal;
}

Mesh* Triangle::getMesh() {return this->mesh;}

RGB Triangle::getColorAtTexture(Triangle& t, Vector3 baryCoords){
    if (t.tex == nullptr) return RGB(0,0,0);

    Texture* tex = t.tex;

    Vector3 uv1 = Vector3(t.uv1.u, t.uv1.v, 0);
    Vector3 uv2 = Vector3(t.uv2.u, t.uv2.v, 0);
    Vector3 uv3 = Vector3(t.uv3.u, t.uv3.v, 0);

    Vector3 mappedUV = uv1*baryCoords.getX() + uv2*baryCoords.getY() + uv3*baryCoords.getZ();

    if (Scene::texFiltering == TextureFiltering::Nearest){
        int row = floor(mappedUV.getY() * tex->height - 1 + 0.5f);
        int column = floor(mappedUV.getX() * tex->width - 1 + 0.5f);
        
        return tex->mat[row * tex->width + column];
    }
    else if (Scene::texFiltering == TextureFiltering::Bilinear){
        float t_x = mappedUV.getX() * (tex->width - 1);
        float t_y = mappedUV.getY() * (tex->height - 1);
        
        int t_x0 = floor(t_x); int t_y0 = floor(t_y);
        int t_x1 = std::min(t_x0 + 1, tex->width - 1); int t_y1 = std::min(t_y0 + 1, tex->height - 1);

        float dx = t_x - t_x0;
        float dy = t_y - t_y0;
      
        Vector3 bL = tex->mat[t_y0 * tex->width + t_x0];
        Vector3 bR = tex->mat[t_y0 * tex->width + t_x1];
        Vector3 tL = tex->mat[t_y1 * tex->width + t_x0];
        Vector3 tR = tex->mat[t_y1 * tex->width + t_x1];
        
        Vector3 cx1 = bL + (bR - bL) * dx;
        Vector3 cx2 = tL + (tR - tL) * dx;
        
        return (RGB) (cx2 + (cx2 - cx1) * dy);
    }
}

void Triangle::move(Vector3 dir){
    printf("aa"); //Implementação futura
}

void Triangle::scale(Vector3 scalingFactors, Vector3 pivot){
    Vector3 localV1 = v1 - pivot;
    localV1 = localV1 * scalingFactors;

    Vector3 localV2 = v2 - pivot;
    localV2 = localV2 * scalingFactors;

    Vector3 localV3 = v3 - pivot;
    localV3 = localV3 * scalingFactors;

    v1 = pivot + localV1;
    v2 = pivot + localV2;
    v3 = pivot + localV3;
}

void Triangle::translate(Vector3 dir){
    v1 = v1 + dir;
    v2 = v2 + dir;
    v3 = v3 + dir;
}

void Triangle::rotateX(float degrees, Vector3 rotCenter) {
    float rads = degrees * M_PI / 180;
    float cosAng = cos(rads);
    float sinAng = sin(rads);
    
    Vector3 localPosV1 = v1 - rotCenter;
    localPosV1 = Vector3(localPosV1.getX(), 
                         localPosV1.getY() * cosAng - localPosV1.getZ() * sinAng, 
                         localPosV1.getY() * sinAng + localPosV1.getZ() * cosAng);
    v1 = rotCenter + localPosV1;

    Vector3 localPosV2 = v2 - rotCenter;
    localPosV2 = Vector3(localPosV2.getX(), 
                         localPosV2.getY() * cosAng - localPosV2.getZ() * sinAng, 
                         localPosV2.getY() * sinAng + localPosV2.getZ() * cosAng);
    v2 = rotCenter + localPosV2;

    Vector3 localPosV3 = v3 - rotCenter;
    localPosV3 = Vector3(localPosV3.getX(), 
                         localPosV3.getY() * cosAng - localPosV3.getZ() * sinAng, 
                         localPosV3.getY() * sinAng + localPosV3.getZ() * cosAng);
    v3 = rotCenter + localPosV3;

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

void Triangle::rotateY(float degrees, Vector3 rotCenter) {
    float rads = degrees * M_PI / 180;
    float cosAng = cos(rads);
    float sinAng = sin(rads);

    Vector3 localPosV1 = v1 - rotCenter;
    localPosV1 = Vector3(localPosV1.getX() * cosAng + localPosV1.getZ() * sinAng, 
                         localPosV1.getY(), 
                         -localPosV1.getX() * sinAng + localPosV1.getZ() * cosAng);
    v1 = rotCenter + localPosV1;

    Vector3 localPosV2 = v2 - rotCenter;
    localPosV2 = Vector3(localPosV2.getX() * cosAng + localPosV2.getZ() * sinAng, 
                         localPosV2.getY(), 
                         -localPosV2.getX() * sinAng + localPosV2.getZ() * cosAng);
    v2 = rotCenter + localPosV2;

    Vector3 localPosV3 = v3 - rotCenter;
    localPosV3 = Vector3(localPosV3.getX() * cosAng + localPosV3.getZ() * sinAng, 
                         localPosV3.getY(), 
                         -localPosV3.getX() * sinAng + localPosV3.getZ() * cosAng);
    v3 = rotCenter + localPosV3;

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

void Triangle::rotateZ(float degrees, Vector3 rotCenter) {
    float rads = degrees * M_PI / 180;
    float cosAng = cos(rads);
    float sinAng = sin(rads);

    Vector3 localPosV1 = v1 - rotCenter;
    localPosV1 = Vector3(localPosV1.getX() * cosAng - localPosV1.getY() * sinAng, 
                         localPosV1.getX() * sinAng + localPosV1.getY() * cosAng, 
                         localPosV1.getZ());
    v1 = rotCenter + localPosV1;

    Vector3 localPosV2 = v2 - rotCenter;
    localPosV2 = Vector3(localPosV2.getX() * cosAng - localPosV2.getY() * sinAng, 
                         localPosV2.getX() * sinAng + localPosV2.getY() * cosAng, 
                         localPosV2.getZ());
    v2 = rotCenter + localPosV2;

    Vector3 localPosV3 = v3 - rotCenter;
    localPosV3 = Vector3(localPosV3.getX() * cosAng - localPosV3.getY() * sinAng, 
                         localPosV3.getX() * sinAng + localPosV3.getY() * cosAng, 
                         localPosV3.getZ());
    v3 = rotCenter + localPosV3;

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

Vector3 Triangle::getPointRotationAroundArbitraryAxis(float degrees, Vector3 point, Vector3 rotCenter, Vector3 axis){
    Vector3 front, right, up;
    front = axis.normalize();
  
    if (front.getX() == 0.0f && front.getY() == 1.0f && front.getZ() == 0.0f) right = Vector3(1,0,0);
    else right = Vector3(0,1,0).cartesianCross(front).normalize();

    up = front.cartesianCross(-right).normalize();

    float cos_theta = std::cos(degrees * M_PI/180.0f);
    float sin_theta = std::sin(degrees * M_PI/180.0f);

    Vector3 Pos = point - rotCenter;
    float dist = Pos.magnitude();
    float F_p = Pos.cartesianDot(front); //Componente F
    float U_p = Pos.cartesianDot(up); //Componente U
    float R_p = Pos.cartesianDot(right); //Componente R

    float U_np = U_p*cos_theta - R_p*sin_theta; //nova posição para eixo U
    float R_np = R_p*cos_theta + U_p*sin_theta; //nova posição para eixo R

    return rotCenter + front * F_p + up * U_np + right * R_np; //Voltando para coordenadas de mundo
}

void Triangle::rotateAroundAxis(float degrees, Vector3 rotCenter, Vector3 axis){
    this->v1 = Triangle::getPointRotationAroundArbitraryAxis(degrees, this->v1, rotCenter, axis);
    this->v2 = Triangle::getPointRotationAroundArbitraryAxis(degrees, this->v2, rotCenter, axis);
    this->v3 = Triangle::getPointRotationAroundArbitraryAxis(degrees, this->v3, rotCenter, axis);

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

Vector3 Triangle::getMirroredPointOverPlane(Vector3 point, Vector3 plane_pos, Vector3 plane_normal){
    plane_normal = plane_normal.normalize(); //Só para garantir que tá normalizado!

    //Vamos converter a posição do ponto para coordenadas do plano
    Vector3 pos = point - plane_pos;
    float proj_on_normal = pos.cartesianDot(plane_normal);

    //Trazendo de volta para coordenadas de mundo
    return point - plane_normal * 2 * proj_on_normal;
}

void Triangle::mirrorOverPlane(Vector3 plane_pos, Vector3 plane_normal){
    this->v1 = Triangle::getMirroredPointOverPlane(this->v1, plane_pos, plane_normal);
    this->v2 = Triangle::getMirroredPointOverPlane(this->v2, plane_pos, plane_normal);
    this->v3 = Triangle::getMirroredPointOverPlane(this->v3, plane_pos, plane_normal);

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

void Triangle::shearX(float sz, float sy, Vector3 pivot) {
    Vector3 localV1 = v1 - pivot;
    localV1 = Vector3(localV1.getX(), localV1.getY() + sy * localV1.getX(), localV1.getZ() + sz * localV1.getX());
    v1 = pivot + localV1;

    Vector3 localV2 = v2 - pivot;
    localV2 = Vector3(localV2.getX(), localV2.getY() + sy * localV2.getX(), localV2.getZ() + sz * localV2.getX());
    v2 = pivot + localV2;

    Vector3 localV3 = v3 - pivot;
    localV3 = Vector3(localV3.getX(), localV3.getY() + sy * localV3.getX(), localV3.getZ() + sz * localV3.getX());
    v3 = pivot + localV3;

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

void Triangle::shearY(float sx, float sz, Vector3 pivot) {
    Vector3 localV1 = v1 - pivot;
    localV1 = Vector3(localV1.getX() + sx * localV1.getY(), localV1.getY(), localV1.getZ() + sz * localV1.getY());
    v1 = pivot + localV1;

    Vector3 localV2 = v2 - pivot;
    localV2 = Vector3(localV2.getX() + sx * localV2.getY(), localV2.getY(), localV2.getZ() + sz * localV2.getY());
    v2 = pivot + localV2;

    Vector3 localV3 = v3 - pivot;
    localV3 = Vector3(localV3.getX() + sx * localV3.getY(), localV3.getY(), localV3.getZ() + sz * localV3.getY());
    v3 = pivot + localV3;

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

void Triangle::shearZ(float sy, float sx, Vector3 pivot) {
    Vector3 localV1 = v1 - pivot;
    localV1 = Vector3(localV1.getX() + sx * localV1.getZ(), localV1.getY() + sy * localV1.getZ(), localV1.getZ());
    v1 = pivot + localV1;

    Vector3 localV2 = v2 - pivot;
    localV2 = Vector3(localV2.getX() + sx * localV2.getZ(), localV2.getY() + sy * localV2.getZ(), localV2.getZ());
    v2 = pivot + localV2;

    Vector3 localV3 = v3 - pivot;
    localV3 = Vector3(localV3.getX() + sx * localV3.getZ(), localV3.getY() + sy * localV3.getZ(), localV3.getZ());
    v3 = pivot + localV3;

    this->standard_normal = (v2 - v1).cartesianCross(v3 - v1).normalize();
}

Vector3 Triangle::getAABBminCoords() {return minCoordsOfTriangle(*this);}
Vector3 Triangle::getAABBmaxCoords() {return maxCoordsOfTriangle(*this);}

//Classe Mesh
void Mesh::computePivot(){ //Aqui o centro geométrico da malha é usado como pivô!
    Vector3 sum(0, 0, 0);
    int vertexCount = 0;

    for (int i = 0; i < facesNumber; ++i) {
        Triangle& t = triangles[i];

        sum = sum + t.v1; 
        sum = sum + t.v2;
        sum = sum + t.v3;

        vertexCount += 3;
    }

    this->pivot = sum/vertexCount;

    this->pivot_x = Vector3(1,0,0);
    this->pivot_y = Vector3(0,1,0);
    this->pivot_z = Vector3(0,0,1);
}

Mesh::Mesh(string name, Vertex* vertices, Face* faces, UV* uv_coords, int facesNumber)
    : Object(name, Vector3(0,0,0), RGB(0,0,0), RGB(0,0,0), RGB(0,0,0), ObjType::TMesh) {
        Triangle* tFaces = new Triangle[facesNumber];
      
        for (int i = 0; i < facesNumber; ++i){
            Face f = faces[i];
            Vertex v1 = vertices[f.v1]; Vertex v2 = vertices[f.v2]; Vertex v3 = vertices[f.v3];
            UV uv1 = uv_coords[f.t1]; UV uv2 = uv_coords[f.t2]; UV uv3 = uv_coords[f.t3];

            tFaces[i] = Triangle(v1, v2, v3, RGB(1,0,0), RGB(1,0,0), RGB(1,1,1));

            if (!f.material.isEmpty){
                tFaces[i].ambientColor = f.material.ambientEnergy;
                tFaces[i].diffuseColor = f.material.diffuseEnergy;
                tFaces[i].specularColor = f.material.specularEnergy;
                tFaces[i].specularExponent = f.material.specularExponent;

                tFaces[i].tex = f.material.loadedTexture;
                tFaces[i].uv1 = uv1; tFaces[i].uv2 = uv2; tFaces[i].uv3 = uv3;
            }

            tFaces[i].mesh = this;
            tFaces[i].id = i;
        }
     
        this->triangles = tFaces;
        this->facesNumber = facesNumber;

        delete[] vertices;
        delete[] faces;
        delete[] uv_coords;

        computePivot();
    }
Mesh::Mesh(Vertex* vertices, Face* faces, UV* uv_coords, int facesNumber)
    : Mesh("Mesh", vertices, faces, uv_coords, facesNumber) {} 
Mesh::~Mesh() {delete[] this->triangles;}

Mesh* Mesh::FromObj(const string& filename){
    Obj_Importer::ImportResult* r = Obj_Importer::Importer::import(filename);

    Vertex* vertices = new Vertex[r->vertices->size()]();
    Face* faces = new Face[r->faces->size()]();
    UV* uv_coords = new UV[r->texCoords->size()];

    //Carregando vértices
    for (int i = 0; i < r->vertices->size(); ++i){
        Obj_Importer::Vertex importedV = r->vertices->at(i);
        Vertex v = Vertex();
        v.x = importedV.x; v.y = importedV.y; v.z = importedV.z;

        vertices[i] = v;
    }
    
    //Carregando coordenadas UV
    for (int i = 0; i < r->texCoords->size(); ++i){
        Obj_Importer::UV importedUV = r->texCoords->at(i);
        UV uv = UV();
        uv.u = importedUV.u; uv.v = importedUV.v;

        uv_coords[i] = uv;
    }

    //Carregando faces
    for (int i = 0; i < r->faces->size(); ++i){
        Obj_Importer::Face importedF = r->faces->at(i);
        Face f = Face();
        f.n1 = importedF.n1; f.n2 = importedF.n2; f.n3 = importedF.n3;
        f.v1 = importedF.v1; f.v2 = importedF.v2; f.v3 = importedF.v3;
        f.t1 = importedF.t1; f.t2 = importedF.t2; f.t3 = importedF.t3;

        f.material.isEmpty = importedF.material.isEmpty;
        f.material.specularExponent = importedF.material.Ns;
        f.material.ambientEnergy = RGB(importedF.material.Ka[0], importedF.material.Ka[1], importedF.material.Ka[2]);
        f.material.diffuseEnergy = RGB(importedF.material.Kd[0], importedF.material.Kd[1], importedF.material.Kd[2]);
        f.material.specularEnergy = RGB(importedF.material.Ks[0], importedF.material.Ks[1], importedF.material.Ks[2]);

        f.material.loadedTexture = importedF.material.loadedTexture; f.material.name = importedF.material.name; f.material.texturePath = importedF.material.texturePath;

        faces[i] = f;   
    }

    int facesSize = r->faces->size();
    delete(r);
    return new Mesh(vertices, faces, uv_coords, facesSize);
}

RaycastHit Mesh::getRaycastHit(Ray ray){
    RaycastHit h = RaycastHit();
    h.functionParameter = std::numeric_limits<float>::max();;

    for (int i = 0; i < this->facesNumber; ++i){
        RaycastHit rH = this->triangles[i].getRaycastHit(ray);

        if (rH.hit && rH.functionParameter < h.functionParameter) h = rH;
    }

    return h;
}

Vector3 Mesh::getNormalAtIntersection(Vector3 intersectionPoint, Ray intersectionRay) {return Vector3(0,0,0);}

int Mesh::getFacesCount() {return this->facesNumber;}
Triangle* Mesh::getFaces() {return this->triangles;}
Vector3 Mesh::getPosition() {return this->pivot;}
Vector3 Mesh::getPivot() {return this->pivot;}

void Mesh::scale(Vector3 scalingFactors){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.scale(scalingFactors, pivot);
    }
}

void Mesh::translate(Vector3 dir){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.translate(dir);
    }

    computePivot();
}

void Mesh::rotateX(float degrees){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.rotateAroundAxis(degrees, pivot, pivot_x);
    }

    pivot_y = Triangle::getPointRotationAroundArbitraryAxis(degrees, pivot_y, Vector3(0,0,0), pivot_x);
    pivot_z = Triangle::getPointRotationAroundArbitraryAxis(degrees, pivot_z, Vector3(0,0,0), pivot_x);

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

void Mesh::rotateY(float degrees){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.rotateAroundAxis(degrees, pivot, pivot_y);
    }

    pivot_x = Triangle::getPointRotationAroundArbitraryAxis(degrees, pivot_x, Vector3(0,0,0), pivot_y);
    pivot_z = Triangle::getPointRotationAroundArbitraryAxis(degrees, pivot_z, Vector3(0,0,0), pivot_y);

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

void Mesh::rotateZ(float degrees){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.rotateAroundAxis(degrees, pivot, pivot_z);
    }

    pivot_x = Triangle::getPointRotationAroundArbitraryAxis(degrees, pivot_x, Vector3(0,0,0), pivot_z);
    pivot_y = Triangle::getPointRotationAroundArbitraryAxis(degrees, pivot_y, Vector3(0,0,0), pivot_z);

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

void Mesh::mirrorX(){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.mirrorOverPlane(pivot, pivot_x);
    }

    computePivot();

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

void Mesh::mirrorY(){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.mirrorOverPlane(pivot, pivot_y);
    }

    computePivot();

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

void Mesh::mirrorZ(){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.mirrorOverPlane(pivot, pivot_z);
    }

    computePivot();

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

void Mesh::shearX(float sz, float sy){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.shearX(sz, sy, pivot);
    }
}

void Mesh::shearY(float sx, float sz){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.shearY(sx, sz, pivot);
    }
}

void Mesh::shearZ(float sy, float sx){
    for (int i = 0; i < facesNumber; ++i){
        Triangle& t = triangles[i];
        t.shearZ(sy, sx, pivot);
    }
}

void Mesh::changeColor(RGB newColor){
    for (int i = 0; i < this->facesNumber; ++i){
        triangles[i].ambientColor = newColor;
        triangles[i].diffuseColor = newColor;
    }
}

void Mesh::move(Vector3 dir){
    this->translate(dir);

    if (this->s_owner != nullptr){
        this->s_owner->requestBVHCaching();
    }
}

void Mesh::setPosition(Vector3 newPos){
    Vector3 moveDir = newPos - this->getPosition();
    this->move(moveDir);
}

Vector3 Mesh::getAABBminCoords() {return Vector3();}
Vector3 Mesh::getAABBmaxCoords() {return Vector3();}

//Classe Light
Light::Light(string name, Vector3 position, RGB colorRGB){
    this->name = (char*) name.c_str();
    this->on = true;

    this->position = position;
    this->colorRGB = colorRGB;

    this->type = LightType::Point;
}
Light::Light(string name, Vector3 position) : Light::Light(name, position, RGB(1,1,1)) {};
Light::Light(string name) : Light::Light(name, Vector3(0,0,0)) {};
Light::Light() : Light::Light("Light", Vector3(0,0,0)) {};

string Light::getName() {return this->name;}
void Light::setName(string newName) {this->name = newName;}

RGB Light::getColor() {return colorRGB;}
void Light::setColor(RGB colorRGB) {this->colorRGB = colorRGB;}

LightType Light::getType() {return type;}

bool Light::isOn() {return on;}
void Light::turnOn() {on = true;}
void Light::turnOff() {on = false;}
bool& Light::getOn_ref() {return on;}

float& Light::getPotency_ref() {return potency;}
float& Light::getSpotAngle_ref() {return spotAngle;}
float& Light::getFocus_ref() {return focus;}

string Light::typeAsString() {
    if (type == LightType::Directional) return "Directional";
    else if (type == LightType::Point) return "Point";
    else if (type == LightType::Spot) return "Spot";

    else return "?";
}

Vector3 Light::getPosition() {return position;}
void Light::move(Vector3 dir){this->position = this->position + dir;}

Light* Light::pointLight(Vector3 position, RGB colorRGB, float potency, string name){
    Light* l = new Light(name);

    l->position = position;
    l->colorRGB = colorRGB;
    l->potency = potency;

    l->type = LightType::Point;

    return l;
}
Light* Light::pointLight(Vector3 position, RGB colorRGB, float potency){ return Light::pointLight(position, colorRGB, potency, "Point Light"); }

Light* Light::directionalLight(Vector3 direction, RGB colorRGB, string name){
    Light* l = new Light(name);

    l->position = -direction.normalize() * Vector3(10e32, 10e32, 10e32);
    l->direction = -direction.normalize();
    l->colorRGB = colorRGB;
    
    l->type = LightType::Directional;

    return l;
}
Light* Light::directionalLight(Vector3 direction, RGB colorRGB) { return Light::directionalLight(direction, colorRGB, "Directional Light"); }

Light* Light::spotLight(Vector3 position, Vector3 direction, float spotAngle, float focus, RGB colorRGB, string name){
    //Ângulo da luz spot deve estar no intervalo (0, 90)
    if (spotAngle <= 0.0f) spotAngle = 10.0f;
    if (spotAngle >= 90.0f) spotAngle = 80.0f;

    Light* l = new Light(name);

    l->position = position;
    l->direction = direction;
    l->spotAngle = spotAngle;
    l->focus = focus;
    l->colorRGB = colorRGB;
    
    l->type = LightType::Spot;
    
    return l;
}
Light* Light::spotLight(Vector3 position, Vector3 direction, float spotAngle, float focus, RGB colorRGB){ return Light::spotLight(position, direction, spotAngle, focus, colorRGB, "Spot Light"); }

bool Light::isShadowed(Scene& scene, RaycastHit hit){
    Vector3 rayDir = (this->position - hit.hitPoint).normalize();

    Ray toLightRay = Ray(hit.hitPoint + rayDir * 0.001f, rayDir); //O 0.001f é um coeficiente de correção para caso o hit.hitPoint coincida com um ponto de um objeto
    float distToLight = (this->position - hit.hitPoint).magnitude();

    RaycastHit tLHit = scene.Raycast(toLightRay);

    if (tLHit.hit && tLHit.functionParameter <= distToLight) return true;
    else return false;
}

RGB Light::evaluateShading(Scene& scene, RaycastHit hit){
    if (this->isShadowed(scene, hit)) return RGB(0,0,0);

    Camera camera = scene.getCamera();
    RGB color;
 
    RGB difColor = hit.object->getDiffuseColor();
    RGB specColor = hit.object->getSpecularColor();

    if (hit.hasTexture) {difColor = hit.textureColor; specColor = hit.textureColor;}

    Vector3 normal = hit.normal;

    Vector3 dir = (hit.hitPoint - camera.getEyePos()).normalize();
    Vector3 incidentRay = (hit.hitPoint - this->position).normalize();
    Vector3 reflectedRay = (normal * (normal.cartesianDot(-incidentRay)) * 2 + incidentRay).normalize();

    RGB diffuseEnergy;
    RGB specularEnergy;

    if (this->type == LightType::Point){
        float dist = (this->position - hit.hitPoint).magnitude();
        float attenuation = potency/(1.0f + 0.09f*dist + 0.032f*dist*dist); //atenuação da luz pontual proporcional à distância

        diffuseEnergy = (Vector3) difColor * this->colorRGB * normal.cartesianDot(-incidentRay) * attenuation;
        specularEnergy = (Vector3) specColor * this->colorRGB * pow(reflectedRay.cartesianDot(-dir), (int)hit.object->getSpecularExponent()) * attenuation;
    }
    else if (this->type == LightType::Directional){
        diffuseEnergy = (Vector3) difColor * this->colorRGB * normal.cartesianDot(-incidentRay);
        specularEnergy = (Vector3) specColor * this->colorRGB * pow(reflectedRay.cartesianDot(-dir), (int)hit.object->getSpecularExponent());
    }
    else if (this->type == LightType::Spot){
        float openingAngleCos = this->direction.cartesianDot(incidentRay);

        if (openingAngleCos < std::cos(this->spotAngle * M_PI/180.0f)){ //Tá fora do cone da spot
            return RGB(0.0f,0.0f,0.0f);
        }
        else{
            float intensity = pow(openingAngleCos, (int)this->focus);

            diffuseEnergy = (Vector3) difColor * this->colorRGB * normal.cartesianDot(-incidentRay) * intensity;
            specularEnergy = (Vector3) specColor * this->colorRGB * pow(reflectedRay.cartesianDot(-dir), (int)hit.object->getSpecularExponent()) * intensity;
        }
    }

    color = (Vector3) diffuseEnergy + specularEnergy;

    return color;
}

//struct AABB
AABB AABB::build(Object& obj){
    AABB box;

    box.min = obj.getAABBminCoords();
    box.max = obj.getAABBmaxCoords();

    return box;
}

AABB AABB::combine(std::vector<int>& indices, std::vector<AABB>& boxes){
    float x_min = INFINITY, y_min = INFINITY, z_min = INFINITY;
    float x_max = -INFINITY, y_max = -INFINITY, z_max = -INFINITY;

    for (int i: indices){
        AABB& box = boxes[i];
        x_min = fminf(x_min, box.min.getX());
        y_min = fminf(y_min, box.min.getY());
        z_min = fminf(z_min, box.min.getZ());

        x_max = fmaxf(x_max, box.max.getX());
        y_max = fmaxf(y_max, box.max.getY());
        z_max = fmaxf(z_max, box.max.getZ());
    }

    AABB box;
    box.min = Vector3(x_min, y_min, z_min);
    box.max = Vector3(x_max, y_max, z_max);

    return box;
}

bool AABB::rayIntersectsAABB(Ray& ray, const AABB& box) {
    float t_min = 0.0f;
    float t_max = std::numeric_limits<float>::max();
    
    for (int axis = 0; axis < 3; ++axis) {

        float ray_origin = ray.getInitialPos().getAxisFromInt(axis);
        float ray_dir = ray.getDirection().getAxisFromInt(axis);
        float box_min = box.min.getAxisFromInt(axis);
        float box_max = box.max.getAxisFromInt(axis);

        if (ray_dir == 0.0f) {

            if (ray_origin < box_min || ray_origin > box_max) {
                return false;
            }
        } else {

            float t1 = (box_min - ray_origin) / ray_dir;
            float t2 = (box_max - ray_origin) / ray_dir;

            if (t1 > t2) {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            t_min = fmaxf(t_min, t1);
            t_max = fminf(t_max, t2);

            if (t_min > t_max) {
                return false;
            }
        }
    }

    return t_max > 0;
}

//struct BVHNode

void BVHNode::__split(std::vector<int>& indices, std::vector<int>& leftIndices, std::vector<int>& rightIndices, std::vector<AABB>& boxes, float splitValue, int axis){
    for (int i: indices){
        float axisCenter = (boxes[i].min.getAxisFromInt(axis) + boxes[i].max.getAxisFromInt(axis))/2.0f;
 
        if (axisCenter < splitValue) leftIndices.push_back(i);
        else rightIndices.push_back(i);
    }
}

BVHNode* BVHNode::__rec_build(std::vector<Object*>& objs, std::vector<int>& indices, std::vector<AABB>& boxes){
    if (indices.size() == 0) return nullptr;

    if (indices.size() == 1){
        BVHNode* node = new BVHNode();
        node->bbox = boxes[indices[0]];
        node->left = nullptr;
        node->right = nullptr;
        node->obj = objs[indices[0]];

        return node;
    }

    AABB fusedBox = AABB::combine(indices, boxes);

    int axis = 0;
    float maxAxisDistance = fusedBox.max.getAxisFromInt(axis) - fusedBox.min.getAxisFromInt(axis);

    for (int i = 1; i < 3; ++i){
        float dist = fusedBox.max.getAxisFromInt(i) - fusedBox.min.getAxisFromInt(i);

        if (dist > maxAxisDistance){
            axis = i;
            maxAxisDistance = dist; 
        }
    }

    std::vector<int> leftIndices, rightIndices;
    float splitValue = (fusedBox.max.getAxisFromInt(axis) + fusedBox.min.getAxisFromInt(axis))/2.0f;

    BVHNode::__split(indices, leftIndices, rightIndices, boxes, splitValue, axis);

    if (leftIndices.empty() || rightIndices.empty()){
        size_t half = indices.size() / 2;
        leftIndices.assign(indices.begin(), indices.begin() + half);
        rightIndices.assign(indices.begin() + half, indices.end());
    }

    BVHNode* node = new BVHNode();
    node->bbox = fusedBox;
    node->left = BVHNode::__rec_build(objs, leftIndices, boxes);
    node->right = BVHNode::__rec_build(objs, rightIndices, boxes);
    node->obj = nullptr;

    return node;
}

BVHNode* BVHNode::build(std::vector<Object*>& objs, int objCount){
    std::vector<AABB> boxes;
    std::vector<int> indices;

    //Construindo as caixas
    for (int i = 0; i < objCount; ++i){
        boxes.push_back(
            AABB::build(*objs[i])
        );

        indices.push_back(i);
    }

    return __rec_build(objs, indices, boxes);
}

RaycastHit BVHNode::rayIntersectsBVH(Ray& ray, BVHNode* root) {
    if (!root) return RaycastHit();

    const int MAX_STACK_SIZE = 64;
    BVHNode* stack[MAX_STACK_SIZE];
    int stackIdx = 0;

    stack[stackIdx++] = root;

    RaycastHit minHit;
    float minParameter = INFINITY;

    while (stackIdx > 0) {
        BVHNode* node = stack[--stackIdx];

        if (AABB::rayIntersectsAABB(ray, node->bbox)) {
            if (node->obj != nullptr) {
                RaycastHit hit = node->obj->getRaycastHit(ray);
            
                if (hit.hit && hit.functionParameter < minParameter){
                    minHit = hit;

                    minParameter = hit.functionParameter;
                }
            } else {
                if (node->left) stack[stackIdx++] = node->left;
                if (node->right) stack[stackIdx++] = node->right;
            }
        }
    }

    return minHit;
}