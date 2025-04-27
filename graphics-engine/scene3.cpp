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
    Engine::Start();

    return 0;
    //Dimensões do canvas
    int screenResWidth = 800;
    int screenResHeight = 800;
    //

    //Setando o olho
    Vector3 eyePos = Vector3(0,0,0);
    //

    //Setando o frame
    float wFrame = 60.0f;
    float hFrame = 60.0f; //wFrame / (screenResWidth/screenResHeight); //Esse cálculo é para o tamanho do frame ficar na mesma proporção da resolução da janela
    float dFrame = 30.0f;

    int nCols = screenResWidth;
    int nRows = screenResHeight;

    Frame f = Frame(wFrame, hFrame, nCols, nRows);
    //

    //Inicializando sdl
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Hello World - SDL2", 
                                          SDL_WINDOWPOS_CENTERED, 
                                          SDL_WINDOWPOS_CENTERED, 
                                          screenResWidth, screenResHeight, 
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    //Inicializando imgui p/sdl
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330");
    //

    Camera c = Camera(eyePos, f, dFrame);
    Scene scene = Scene(c, window);

    // Mesh* m_tampo = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* m_pe1 = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* m_pe2 = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* surface = Mesh::FromObj("importedObjects/cube.obj");

    // surface->scale(Vector3(15.0f, 0.1f, 15.0f));
    // surface->translate(Vector3(0.0f, -0.95 - 0.1f, 0.0f));
    // surface->changeColor(RGB(0.584f, 0.949f, 0.584f));
    // scene.addObject(surface);
    
    // m_tampo->scale(Vector3(2.5f/2.0f, 0.05f/2.0f, 1.5f/2.0f));
    // m_tampo->changeColor(RGB(0.369f, 0.369f, 0.369f));
    
    // m_pe1->scale(Vector3(0.05f/2.0f, 0.95f/2.0f, 1.5f/2.0f));
    // m_pe1->translate(Vector3(-2.5f/2.0f + 0.05f/2, -0.95f/2.0f - 0.05f/2.0f, 0.0f));
    // m_pe1->changeColor(RGB(0.212f, 0.145f, 0));

    // m_pe2->scale(Vector3(0.05f/2.0f, 0.95f/2.0f, 1.5f/2.0f));
    // m_pe2->translate(Vector3(2.5f/2.0f - 0.05f/2, -0.95f/2.0f - 0.05f/2.0f, 0.0f));
    // m_pe2->changeColor(RGB(0.212f, 0.145f, 0));

    // scene.addObject(m_tampo);
    // scene.addObject(m_pe1);
    // scene.addObject(m_pe2);

    // Mesh* a_bola = Mesh::FromObj("importedObjects/ball.obj");
    // Mesh* a_cone = Mesh::FromObj("importedObjects/cone.obj");
    // Mesh* a_tronco = Mesh::FromObj("importedObjects/cylinder.obj");
    // Mesh* a_suporte = Mesh::FromObj("importedObjects/cylinder.obj");

    // a_bola->scale(Vector3(0.045f, 0.045f, 0.045f));
    // a_bola->translate(Vector3(0, 1.5f/2.0f + 0.05f/2 + 0.09f + 0.4f + 1.5f/2.0f + 0.045f/2.0f, -0.045f/3.0f));
    // a_bola->changeColor(RGB(1, 0.984f, 0));

    // a_tronco->scale(Vector3(0.06f, 0.4f/2.0f, 0.06f));
    // a_tronco->translate(Vector3(0, 0.4f/2.0f + 0.05f/2.0f + 0.09f, 0));
    // a_tronco->changeColor(RGB(0.31f, 0.212f, 0));

    // a_suporte->scale(Vector3(0.3f, 0.09f/2.0f, 0.3f));
    // a_suporte->translate(Vector3(0, 0.09f/2.0f + 0.05f/2.0f, 0));

    // a_cone->scale(Vector3(0.6f, 1.5f/2.0f, 0.6f));
    // a_cone->translate(Vector3(0, 0.05f/2.0f + 0.09f + 0.4f + 1.7f/2.0f, 0));
    // a_cone->changeColor(RGB(0.0f, 1.0f, 0.0f));

    // scene.addObject(a_bola);
    // scene.addObject(a_cone);
    // scene.addObject(a_tronco);
    // scene.addObject(a_suporte);

    // Mesh* pf_colunaesq = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* pf_colunadir = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* pf_vigaesq = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* pf_vigadir = Mesh::FromObj("importedObjects/cube.obj");
    
    // pf_colunaesq->scale(Vector3(0.5f/2.0f, 5.0f/2.0f, 0.3f/2.0f));
    // pf_colunaesq->translate(Vector3(-3.0f - 0.5f/2.0f, 5.0f/2.0f - 0.95f - 0.05f, 5.0f + 0.15f));
    // pf_colunaesq->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // pf_colunadir->scale(Vector3(0.5f/2.0f, 5.0f/2.0f, 0.3f/2.0f));
    // pf_colunadir->translate(Vector3(3.0f + 0.5f/2.0f, 5.0f/2.0f - 0.95f - 0.05f, 5.0f + 0.15f));
    // pf_colunadir->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // pf_vigaesq->scale(Vector3(3.0f/2.0f, 0.5f/2.0f, 0.3f/2.0f));
    // pf_vigaesq->shearX(0.0f, 0.75f);
    // pf_vigaesq->translate(Vector3(-3.0f + 1.5f, 5.0f - 0.125f, 5.0f + 0.15f));
    // pf_vigaesq->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // pf_vigadir->scale(Vector3(3.0f/2.0f, 0.5f/2.0f, 0.3f/2.0f));
    // pf_vigadir->shearX(0.0f, 0.75f);
    // pf_vigadir->rotateY(180.0f);
    // pf_vigadir->translate(Vector3(3.0f - 1.5f, 5.0f - 0.125f, 5.0f + 0.15f));
    // pf_vigadir->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // scene.addObject(pf_vigadir);
    // scene.addObject(pf_colunadir);
    // scene.addObject(pf_colunaesq);
    // scene.addObject(pf_vigaesq);
    // scene.addObject(pf_vigadir);

    // Mesh* pt_colunaesq = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* pt_colunadir = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* pt_vigaesq = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* pt_vigadir = Mesh::FromObj("importedObjects/cube.obj");
    
    // pt_colunaesq->scale(Vector3(0.5f/2.0f, 5.0f/2.0f, 0.3f/2.0f));
    // pt_colunaesq->translate(Vector3(-3.0f - 0.5f/2.0f, 5.0f/2.0f - 0.95f - 0.05f, - 5.0f - 0.15f));
    // pt_colunaesq->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // pt_colunadir->scale(Vector3(0.5f/2.0f, 5.0f/2.0f, 0.3f/2.0f));
    // pt_colunadir->translate(Vector3(3.0f + 0.5f/2.0f, 5.0f/2.0f - 0.95f - 0.05f, - 5.0f - 0.15f));
    // pt_colunadir->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // pt_vigaesq->scale(Vector3(3.0f/2.0f, 0.5f/2.0f, 0.3f/2.0f));
    // pt_vigaesq->shearX(0.0f, 0.75f);
    // pt_vigaesq->translate(Vector3(-3.0f + 1.5f, 5.0f - 0.125f, - 5.0f - 0.15f));
    // pt_vigaesq->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // pt_vigadir->scale(Vector3(3.0f/2.0f, 0.5f/2.0f, 0.3f/2.0f));
    // pt_vigadir->shearX(0.0f, 0.75f);
    // pt_vigadir->rotateY(180.0f);
    // pt_vigadir->translate(Vector3(3.0f - 1.5f, 5.0f - 0.125f, - 5.0f - 0.15f));
    // pt_vigadir->changeColor(RGB(161.0f/255.0f, 102/255.0f, 47/255.0f));

    // scene.addObject(pt_vigadir);
    // scene.addObject(pt_colunadir);
    // scene.addObject(pt_colunaesq);
    // scene.addObject(pt_vigaesq);
    // scene.addObject(pt_vigadir);

    // Mesh* g_telhadoesq = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* g_telhadodir = Mesh::FromObj("importedObjects/cube.obj");

    // g_telhadoesq->scale(Vector3(4.0f/2.0f, 0.35f/2.0f, 10.0f/2.0f));
    // g_telhadoesq->rotateZ(-36.8699f);
    // g_telhadoesq->translate(Vector3(3.0f - 1.5f, 5.0f - 0.3f + 0.2f, 0.0f));
    // g_telhadoesq->changeColor(RGB(0.38f, 0, 0.69f));

    // g_telhadodir->scale(Vector3(4.0f/2.0f, 0.35f/2.0f, 10.0f/2.0f));
    // g_telhadodir->rotateZ(36.8699f);
    // g_telhadodir->translate(Vector3(-3.0f + 1.5f, 5.0f - 0.3f + 0.2f, 0.0f));
    // g_telhadodir->changeColor(RGB(0.38f, 0, 0.69f));

    // scene.addObject(g_telhadoesq);
    // scene.addObject(g_telhadodir);

    // Mesh* g_paredeesq = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* g_parededir = Mesh::FromObj("importedObjects/cube.obj");
    // Mesh* g_paredetras = Mesh::FromObj("importedObjects/cube.obj");

    // g_paredeesq->scale(Vector3(0.2f/2.0f, 5.0f/2.0f, 10.0f/2.0f));
    // g_paredeesq->translate(Vector3(-3.0f - 0.5f/2.0f, 5.0f/2.0f - 0.95f - 0.05f, 0));
    // g_paredeesq->changeColor(RGB(0.576f, 0.537f, 1));

    // g_parededir->scale(Vector3(0.2f/2.0f, 5.0f/2.0f, 10.0f/2.0f));
    // g_parededir->translate(Vector3(3.0f + 0.5f/2.0f, 5.0f/2.0f - 0.95f - 0.05f, 0));
    // g_parededir->changeColor(RGB(0.576f, 0.537f, 1));

    // g_paredetras->scale(Vector3(6.0f/2.0f, 5.0f/2.0f, 0.2f/2.0f));
    // g_paredetras->translate(Vector3(0.0f, 5.0f/2.0f - 0.95f - 0.05f, - 5.0f - 0.15f));
    // g_paredetras->changeColor(RGB(0.576f, 0.537f, 1));

    // Sphere* s = new Sphere("esfera", Vector3(10.0f, 30.0f, 0.0f), 10.0f, RGB(0.576f, 0.537f, 1), RGB(0.576f, 0.537f, 1), RGB(0.576f, 0.537f, 1));
    // scene.addObject(s);

    // scene.addObject(g_paredeesq);
    // scene.addObject(g_parededir);
    // scene.addObject(g_paredetras);

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

    Mesh* sofa = Mesh::FromObj("importedObjects/sofa.obj");
    sofa->move(Vector3(0.0f, 20.0f, 0.0f));
    sofa->scale(Vector3(20.0f, 20.0f, 20.0f));
    scene.addObject(sofa);

    Light* l1 = Light::directionalLight(Vector3(0.0f, -0.2f, -1.0f), RGB(0.82f, 0.522f, 0));
    Light* l2 = Light::spotLight(Vector3(4.0f, 2.0f, 10.0f), Vector3(0.0f, 0.0f, -1.0f), 45.0f, 20.0f, RGB(1.0f, 0.0f, 0.0f));

    scene.addLight(l1);
    //scene.addLight(l2);
    //scene.addObject(m);
    //

    scene.renderScene();
    // Main loop
    bool running = true;
    SDL_Event event;
    Object* selectedObject = nullptr;

    int velUP = 0.0f;
    int velRIGHT = 0.0f;
    int velFRONT = 0.0f;

    int speed = 3.0f;

    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;
    std::chrono::duration<double> duration;

    bool IS_WIREFRAME_ENABLED = true;
    scene.changeRenderizationMode(RenderizationMode::Wireframe);

    bool IS_OPTIMIZATION_ENABLED = true;
    scene.switchOptimizedRenderization();

    bool IS_BILINEAR_ENABLED = true;
    scene.changeTextureFiltering(TextureFiltering::Bilinear);

    bool IS_GUI_HIDED = false;

    static char name_buffer[128] = "\0";
    static float pos_buffer[3] = {INFINITY, INFINITY, INFINITY};
    while (running) {
        start = std::chrono::high_resolution_clock::now();

        ImVec2 screenSize = ImGui::GetMainViewport()->Size;

        const Uint8 *state = SDL_GetKeyboardState(NULL);

        if (state[SDL_SCANCODE_W]) velFRONT = 1.0f;
        else if (state[SDL_SCANCODE_S]) velFRONT = -1.0f;
        else velFRONT = 0.0f;

        if (state[SDL_SCANCODE_D]) velRIGHT = 1.0f;
        else if (state[SDL_SCANCODE_A]) velRIGHT = -1.0f;
        else velRIGHT = 0.0f;

        if (state[SDL_SCANCODE_Z]) velUP = 1.0f;
        else if (state[SDL_SCANCODE_C]) velUP = -1.0f;
        else velUP = 0.0f;

        Frame& f = scene.getCamera().getFrame();
        
        if (!io.WantCaptureMouse) { scene.getCamera().move(
            (f.getFrontDir() * velFRONT + f.getRightDir() * velRIGHT + f.getUpDir() * velUP) * speed * DELTA_TIME
        ); }

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if(io.WantCaptureMouse) break; //Se o mouse estiver sobre alguma GUI, ignora os eventos de pressionamento de botão

            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_MOUSEBUTTONUP){
                if (event.button.button == SDL_BUTTON_LEFT){
                    selectedObject = scene.pick(event.button.x, event.button.y);
                    if (selectedObject){
                        std::cout << "Selected Object: " << selectedObject->getName() << '\n';
                    }
                    else std::cout << "Nothing was picked!" << '\n';

                    //Resetando os buffer de seleção
                    strcpy(name_buffer, "\0");

                    pos_buffer[0] = INFINITY; pos_buffer[1] = INFINITY; pos_buffer[2] = INFINITY;
                }
            }
            else if (event.type == SDL_MOUSEMOTION){
                Uint32 buttons = SDL_GetMouseState(NULL, NULL);
                if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
                    scene.getCamera().rotateY(-event.motion.xrel);
                    scene.getCamera().rotateXZ(-event.motion.yrel);
                }
            }
            else if (event.type == SDL_KEYDOWN){
                if (event.key.keysym.sym == SDLK_UP){ //Ir para esquerda
                    //scene.renderScene();
                    //m->shearY(1.0f, 0.0f);
                }
                else if (event.key.keysym.sym == SDLK_DOWN){ //Ir para esquerda
                    //m->shearY(-1.0f, 0.0f);
                }
                else if (event.key.keysym.sym == SDLK_RIGHT){ //Ir para esquerda
                    //m->rotateY(2.0f);
                }
                else if (event.key.keysym.sym == SDLK_LEFT){ //Ir para esquerda
                    //m->rotateY(-2.0f);
                }
                else if (event.key.keysym.sym == SDLK_o){ //Ir para esquerda
                    selectedObject->move(Vector3(0.0f, 1.0f, 0.0f));
                }
                else if (event.key.keysym.sym == SDLK_l){ //Ir para esquerda
                    selectedObject->move(Vector3(0.0f, -1.0f, 0.0f));
                }
                else if (event.key.keysym.sym == SDLK_m){
                    if (!IS_WIREFRAME_ENABLED) { IS_WIREFRAME_ENABLED = true; Scene::changeRenderizationMode(RenderizationMode::Wireframe); }
                    else { IS_WIREFRAME_ENABLED = false; Scene::changeRenderizationMode(RenderizationMode::Default); }
                }
                else if (event.key.keysym.sym == SDLK_n){
                    if (IS_OPTIMIZATION_ENABLED) IS_OPTIMIZATION_ENABLED = false;
                    else IS_OPTIMIZATION_ENABLED = true;

                    Scene::switchOptimizedRenderization();
                }
                else if (event.key.keysym.sym == SDLK_h){
                    IS_GUI_HIDED = !IS_GUI_HIDED;
                }
            }
        }

        scene.renderScene();

        if (!IS_GUI_HIDED){
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            //Options
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_Always);
            ImGui::Begin("Options");

            if(ImGui::Checkbox("WireFrame", &IS_WIREFRAME_ENABLED)){
                if (IS_WIREFRAME_ENABLED) Scene::changeRenderizationMode(RenderizationMode::Wireframe);
                else Scene::changeRenderizationMode(RenderizationMode::Default);
            };

            if(ImGui::Checkbox("Optimized", &IS_OPTIMIZATION_ENABLED)){
                Scene::switchOptimizedRenderization();
            };

            ImGui::SliderInt("##", &scene.optimizationLevel, 2, 10);  // Range from 0 to 100

            if(ImGui::Checkbox("Bilinear Filtering", &IS_BILINEAR_ENABLED)){
                if (IS_BILINEAR_ENABLED) Scene::changeTextureFiltering(TextureFiltering::Bilinear);
                else Scene::changeTextureFiltering(TextureFiltering::Nearest);
            }

            ImGui::End();
            //

            //Selected Object
            if (selectedObject != nullptr){
                ImVec2 windowSize = ImVec2(300, 200);
                ImVec2 windowPos = ImVec2(screenSize.x - windowSize.x - 10, screenSize.y - windowSize.y - 10); // 10px padding

                ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
                ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
                ImGui::Begin("Selected");

                //Name field
                if (strcmp(name_buffer, "\0") == 0){
                    strcpy(name_buffer, selectedObject->getName().c_str());
                }

                if (strcmp(name_buffer, selectedObject->getName().c_str()) != 0){ //Nome foi atualizado no input box
                    selectedObject->setName(name_buffer);
                }

                ImGui::Text("Name"); 
                ImGui::SameLine();        
                ImGui::InputText("##hidden", name_buffer, IM_ARRAYSIZE(name_buffer));

                //Position field
                Vector3 objPosition = selectedObject->getPosition();

                if (pos_buffer[0] == INFINITY){ //não foi inicializado ainda
                    pos_buffer[0] = objPosition.getX();
                    pos_buffer[1] = objPosition.getY();
                    pos_buffer[2] = objPosition.getZ();
                }
                
                if (pos_buffer[0] != objPosition.getX() || pos_buffer[1] != objPosition.getY() || pos_buffer[2] != objPosition.getZ()){ //Posição atualizada
                    Vector3 newPos = Vector3(pos_buffer[0], pos_buffer[1], pos_buffer[2]);
                    Vector3 dir = newPos - objPosition;
            
                    selectedObject->move(dir);
                }

                ImGui::Text("Position");
                ImGui::SameLine();
                ImGui::InputFloat3("", pos_buffer);

                //Remove Button
                if (ImGui::Button("Remove")){
                    scene.removeObject(selectedObject);
                    selectedObject = nullptr;
                }

                ImGui::End();
            }
            //

            //Scene objects
            ImVec2 windowSize = ImGui::GetIO().DisplaySize;

            ImVec2 containerSize = ImVec2(300, 200);
            ImVec2 position = ImVec2(windowSize.x - containerSize.x, 0);

            ImGui::SetNextWindowPos(position);
            ImGui::SetNextWindowSize(containerSize);

            ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            
            int idx = 0;
            for (Object* o: scene.getObjects()){
                if (ImGui::Selectable((o->getName() + "##" + std::to_string(idx)).c_str(), selectedObject == o)){
                    selectedObject = o;

                    //Resetando os buffer de seleção
                    strcpy(name_buffer, "\0");

                    pos_buffer[0] = INFINITY; pos_buffer[1] = INFINITY; pos_buffer[2] = INFINITY;
                }

                ++idx;
            }

            ImGui::EndChild();
            ImGui::End();
            //

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        SDL_GL_SwapWindow(window);

        end = std::chrono::high_resolution_clock::now();

        duration = end - start;
        DELTA_TIME = duration.count();

        char title[32];
        sprintf(title, "Fps: %.1f", 1.0/DELTA_TIME);
        SDL_SetWindowTitle(window, title);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}