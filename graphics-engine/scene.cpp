#define SDL_MAIN_HANDLED

#include "include/graphics-engine.h"

#include "../sdlbuild/include/SDL2/SDL.h"
#include "../glew/glew/include/GL/glew.h"

#include "../imgui/include/imgui.h"
#include "../imgui/include/imgui_impl_sdl2.h"
#include "../imgui/include/imgui_impl_sdlrenderer2.h"
#include "../imgui/include/imgui_impl_opengl3.h"

#include <stdio.h>
#include <iostream>

#include <omp.h>

#include <chrono>
#include <thread>

#include <math.h>

double DELTA_TIME = 1.0f;

int main(int argc, char* argv[]) {
    //Dimensões do canvas
    int screenResWidth = 600;
    int screenResHeight = 600;
    //

    //Setando o olho
    Vector3 eyePos = Vector3(0,0,0);
    //

    //Setando o frame
    float wFrame = 60.0f;
    float hFrame = 60.0f;
    float dFrame = 30.0f;

    int nCols = screenResWidth;
    int nRows = screenResHeight;

    Frame f = Frame(wFrame, hFrame, nCols, nRows);
    //
    
    Camera c = Camera(eyePos, f, dFrame);
    Scene scene = Scene(c);

    Mesh* house = Mesh::FromObj("importedObjects/house.obj");
    house->setName("house");
    scene.addObject(house);

    Mesh* car = Mesh::FromObj("importedObjects/car1.obj");
    car->setName("car");
    car->setPosition(Vector3(2.361f, 0.446f, 1.5f));
    scene.addObject(car);

    Mesh* grassfloor = Mesh::FromObj("importedObjects/grassfloor.obj");
    grassfloor->setName("grassfloor");
    grassfloor->setPosition(Vector3(0.0f, -3.7f, 0.0f));
    scene.addObject(grassfloor);

    Mesh* cube = Mesh::FromObj("importedObjects/cube.obj");
    cube->move(Vector3(30.0f, 20.0f, 0.0f));
    cube->scale(Vector3(5.0f, 5.0f, 5.0f));
    scene.addObject(cube);

    //Snowman
    Sphere* snow_head = new Sphere(Vector3(0,0,0), 0.6f, Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1));
    snow_head->setName("snow_head");
    snow_head->move(Vector3(15.0f, 3.0f, 0.0f));
    scene.addObject(snow_head);

    Sphere* snow_eye1 = new Sphere(Vector3(0,0,0), 0.075f, Vector3(0,0,0), Vector3(0,0,0), Vector3(0,0,0));
    snow_eye1->setName("snow_eye1");
    snow_eye1->move(Vector3(15.2f, 3.0f, 0.53f));
    scene.addObject(snow_eye1);

    Sphere* snow_eye2 = new Sphere(Vector3(0,0,0), 0.075f, Vector3(0,0,0), Vector3(0,0,0), Vector3(0,0,0));
    snow_eye2->setName("snow_eye2");
    snow_eye2->move(Vector3(14.8f, 3.0f, 0.53f));
    scene.addObject(snow_eye2);

    Sphere* snowbody_mid = new Sphere(Vector3(0,0,0), 0.8f, Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1));
    snowbody_mid->setName("snowbody_mid");
    snowbody_mid->move(Vector3(15.0f, 2.0f, 0.0f));
    scene.addObject(snowbody_mid);
    
    Sphere* snowbody_low = new Sphere(Vector3(0,0,0), 1.0f, Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1));
    snowbody_low->setName("snowbody_low");
    snowbody_low->move(Vector3(15.0f, 1.0f, 0.0f));
    scene.addObject(snowbody_low);

    Cone* snow_hat = new Cone(Vector3(0,0,0), Vector3(0,1,0), 0.65f, 1.0f, RGB(0.73f,0.55,0.39), RGB(0.73f,0.55,0.39), RGB(0.73f,0.55,0.39));
    snow_hat->setName("snow_hat");
    snow_hat->move(Vector3(15.0f, 3.25f, 0.0f));
    scene.addObject(snow_hat);

    Cylinder* snow_armRight = new Cylinder(Vector3(0,0,0), Vector3(1,1,0), 0.2f, 1.5f, RGB(1,1,1), RGB(1,1,1), RGB(1,1,1));
    snow_armRight->setName("snow_armRight");
    snow_armRight->move(Vector3(13.5f, 1.35f, 0.0f));
    scene.addObject(snow_armRight);

    Cylinder* snow_armLeft = new Cylinder(Vector3(0,0,0), Vector3(-1,1,0), 0.2f, 1.5f, RGB(1,1,1), RGB(1,1,1), RGB(1,1,1));
    snow_armLeft->setName("snow_armLeft");
    snow_armLeft->move(Vector3(16.5f, 1.35f, 0.0f));
    scene.addObject(snow_armLeft);
    //

    Light* l1 = Light::directionalLight(Vector3(0.0f, -0.2f, -1.0f), RGB(1.0f, 1.0f, 1.0f));
    Light* l2 = Light::spotLight(Vector3(15.0f, 10.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), 15.0f, 0.0f, RGB(9/255.0f,255/255.0f,74/255.0f));
    l2->turnOff();

    Light* l3 = Light::pointLight(Vector3(15.0f, 2.5f, 2.0f), RGB(1.0f, 1.0f, 1.0f), 1.0f);
    l3->turnOff();
    
    scene.addLight(l1);
    scene.addLight(l2);
    scene.addLight(l3);
    //scene.addObject(m);
    //

    Engine::Start(scene, screenResWidth, screenResWidth, 800, 800);

    return 0;
}