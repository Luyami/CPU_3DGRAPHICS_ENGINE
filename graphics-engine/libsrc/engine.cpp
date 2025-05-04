#include "../include/engine.h"

#include <string>
#include <iostream>
#include <SDL_syswm.h>

SDL_Window* Engine::config_window = nullptr;
SDL_Window* Engine::scene_window = nullptr;
Scene* Engine::scene = nullptr;
bool Engine::onLoop = false;
double Engine::DELTA_TIME = 1.0f;

double Engine::velFRONT = 0.0f;
double Engine::velRIGHT = 0.0f;
double Engine::velUP = 0.0f;
double Engine::speed = 8.0f;

int Engine::RT_RAYS = 0;

bool Engine::IS_WIREFRAME_ENABLED = true;
bool Engine::IS_OPTIMIZATION_ENABLED = true;
bool Engine::IS_BILINEAR_ENABLED = true;
bool Engine::USING_ORTHOGRAPHIC_PROJECTION = false;
bool Engine::USING_OBLIQUE_PROJECTION = false;

bool Engine::DRAWING = false;
float Engine::PEN_COLOR[3] = {1.0f, 0.0f, 0.0f};
float Engine::PEN_THICKNESS = 2.0f;
float Engine::LIGHT_COLOR_RGB[3] = {-1.0f, -1.0f, -1.0f};

bool Engine::MOUSE_LEFTB_HELD = false;
char Engine::name_buffer[128] = "\0";
float Engine::pos_buffer[3] = {INFINITY, INFINITY, INFINITY};
float Engine::camera_fov = 45.0f;
Object* Engine::selectedObject = nullptr;
Light* Engine::selectedLight = nullptr;

void Engine::__init_ConfigWindow(Scene& scene, int w, int h){
    SDL_Window* window;
    SDL_Renderer* renderer;

    window = SDL_CreateWindow("Engine Configurations", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    Engine::config_window = window;
}

void Engine::__init_SceneWindow(Scene& scene, int w, int h){
    SDL_Window* window;

    Engine::scene_window = SDL_CreateWindow("Engine", 
                                          SDL_WINDOWPOS_CENTERED, 
                                          SDL_WINDOWPOS_CENTERED, 
                                          w, h,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS);
    if (!Engine::scene_window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        throw std::exception();
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(Engine::scene_window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        throw std::exception();
    }

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        throw std::exception();
    }

    window = Engine::scene_window;
    scene.setCanvas(window);
}

void Engine::__event_step(SDL_Event& event){
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
        Engine::onLoop = false;
    }
    else if (event.type == SDL_MOUSEMOTION && !DRAWING){
        Uint32 buttons = SDL_GetMouseState(NULL, NULL);
        if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
            scene->getCamera().rotateY(-event.motion.xrel);
            scene->getCamera().rotateXZ(-event.motion.yrel);
        }
    }
    else if ((SDL_GetWindowFlags(Engine::getSceneWindow()) & SDL_WINDOW_INPUT_FOCUS) && event.type == SDL_MOUSEWHEEL && !DRAWING){
        //Dar zoom em um lugar específico
        camera_fov -= event.wheel.y * 2.5f;
        
        if (camera_fov > 179.0f) camera_fov = 179.0f;
        else if (camera_fov < 1.0f) camera_fov = 1.0f;

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY); 

        Frame& f = scene->getCamera().getFrame();
        Vector3 pixelPos = scene->getCamera().getFrame().getPixelPos(mouseY, mouseX);

        Engine::scene->getCamera().LookAt(pixelPos);
        Engine::scene->getCamera().changeVerticalFov(camera_fov);

        SDL_WarpMouseInWindow(Engine::getSceneWindow(), f.getColumns()/2.0f, f.getRows()/2.0f);
    }
    else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MINIMIZED){
        SDL_SysWMinfo syswmInfo;
        SDL_VERSION(&syswmInfo.version);
     
        if (SDL_GetWindowWMInfo(Engine::getSceneWindow(), &syswmInfo)) {
            HWND hwnd = syswmInfo.info.win.window;
    
            ShowWindow(hwnd, SW_MINIMIZE);
        }
    }
    else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESTORED){
        SDL_SysWMinfo syswmInfo;
        SDL_VERSION(&syswmInfo.version);
     
        if (SDL_GetWindowWMInfo(Engine::getSceneWindow(), &syswmInfo)) {
            HWND hwnd = syswmInfo.info.win.window;
    
            ShowWindow(hwnd, SW_RESTORE);
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP && !DRAWING){
        if ((SDL_GetWindowFlags(Engine::getSceneWindow()) & SDL_WINDOW_INPUT_FOCUS) && event.button.button == SDL_BUTTON_LEFT){ //Picking só funciona se a tela focada for a da cena
            selectedObject = scene->pick(event.button.x, event.button.y);
            if (selectedObject){
                std::cout << "Selected Object: " << selectedObject->getName() << '\n';
                selectedLight = nullptr;
            }
            else std::cout << "Nothing was picked!" << '\n';

            //Resetando os buffer de seleção
            strcpy(name_buffer, "\0");

            pos_buffer[0] = INFINITY; pos_buffer[1] = INFINITY; pos_buffer[2] = INFINITY;
            LIGHT_COLOR_RGB[0] = -1; LIGHT_COLOR_RGB[1] = -1; LIGHT_COLOR_RGB[2] = -1;
        }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) MOUSE_LEFTB_HELD = true;
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) MOUSE_LEFTB_HELD = false;
    else if (event.type == SDL_MOUSEMOTION && MOUSE_LEFTB_HELD && (SDL_GetWindowFlags(Engine::getSceneWindow()) & SDL_WINDOW_INPUT_FOCUS) && DRAWING){ //Desenhar na tela
            Frame& f = scene->getCamera().getFrame();

            glLineWidth(PEN_THICKNESS);

            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY); 

            glBegin(GL_LINES);
            glColor3f(PEN_COLOR[0], PEN_COLOR[1], PEN_COLOR[2]);
            glVertex2f(-1.0f + (2.0f * (mouseX - event.motion.xrel)) / f.getColumns(), 1.0f - (2.0f * (mouseY+1 - event.motion.yrel)) / f.getRows());
            glVertex2f(-1.0f + (2.0f * mouseX) / f.getColumns(), 1.0f - (2.0f * (mouseY+1)) / f.getRows());

            glEnd();  
    }
}

void Engine::__interface_step(){
    SDL_Window* window = getConfigWindow();
    SDL_Renderer* renderer = SDL_GetRenderer(window); 

    ImVec2 screenSize = ImGui::GetMainViewport()->Size;

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    //Options
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Always);
    ImGui::Begin("Options");

    if(ImGui::Checkbox("WireFrame", &IS_WIREFRAME_ENABLED)){
        if (IS_WIREFRAME_ENABLED) Scene::changeRenderizationMode(RenderizationMode::Wireframe);
        else Scene::changeRenderizationMode(RenderizationMode::Default);
    };

    if(ImGui::Checkbox("Optimized", &IS_OPTIMIZATION_ENABLED)){
        Scene::switchOptimizedRenderization();
    };

    ImGui::SameLine();
    ImGui::Checkbox("Predict", &scene->should_predict_pixel);

    ImGui::SliderInt("##", &scene->optimizationLevel, 2, 10);  // Range from 0 to 100

    if(ImGui::Checkbox("Bilinear Filtering", &IS_BILINEAR_ENABLED)){
        if (IS_BILINEAR_ENABLED) Scene::changeTextureFiltering(TextureFiltering::Bilinear);
        else Scene::changeTextureFiltering(TextureFiltering::Nearest);
    }

    ImGui::Text("RT");
    ImGui::SameLine();
    ImGui::SliderInt("##1", &scene->rt_rays, 0, 5);

    ImGui::End();
    //

    //Camera
    ImGui::SetNextWindowPos(ImVec2(0, 220), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Always);
    ImGui::Begin("Camera");

    if (ImGui::Checkbox("Orthographic Projection", &USING_ORTHOGRAPHIC_PROJECTION)){
        if (USING_ORTHOGRAPHIC_PROJECTION) {Scene::changeCameraProjection(CameraProjection::Orthographic); USING_OBLIQUE_PROJECTION = false;}
        else Scene::changeCameraProjection(CameraProjection::Perspective);
    }

    if (ImGui::Checkbox("Oblique Projection", &USING_OBLIQUE_PROJECTION)){
        if (USING_OBLIQUE_PROJECTION) {Scene::changeCameraProjection(CameraProjection::Oblique); USING_ORTHOGRAPHIC_PROJECTION = false;}
        else Scene::changeCameraProjection(CameraProjection::Perspective);
    }

    if (USING_ORTHOGRAPHIC_PROJECTION){
        //ImGui::SliderFloat("Scale##1", &scene->getCamera().orthogonal_frame_scale, 0.1f, 2.0f);
    }
    else if (USING_OBLIQUE_PROJECTION){
        //ImGui::SliderFloat("Scale##2", &scene->getCamera().oblique_frame_scale, 0.1f, 50.0f);
        ImGui::SliderFloat("Inclination", &scene->getCamera().oblique_inclination_in_degrees, 10.0f, 80.0f);
    }
    else{ //Projeção perspectiva
        if (ImGui::SliderFloat("FOV", &camera_fov, 1.0f, 179.0f)){
            Engine::scene->getCamera().changeVerticalFov(camera_fov);
        }
    }

    ImGui::End();

    //Drawing
    ImGui::SetNextWindowPos(ImVec2(0, 440), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Always);
    ImGui::Begin("Drawing");

    ImGui::Checkbox("Drawing", &DRAWING);

    ImGui::ColorEdit3("Pen Color", PEN_COLOR);

    ImGui::SliderFloat("Pen Size", &PEN_THICKNESS, 0.5f, 5.0f);

    ImGui::End();
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
    for (Object* o: scene->getObjects()){
        if (ImGui::Selectable((o->getName() + "##" + std::to_string(idx)).c_str(), selectedObject == o)){
            selectedObject = o;
            selectedLight = nullptr;

            //Resetando os buffer de seleção
            strcpy(name_buffer, "\0");

            pos_buffer[0] = INFINITY; pos_buffer[1] = INFINITY; pos_buffer[2] = INFINITY;
            LIGHT_COLOR_RGB[0] = -1;
        }

        ++idx;
    }

    for (Light* l: scene->getLightSources()){
        if (ImGui::Selectable((l->getName() + "##" + std::to_string(idx)).c_str(), selectedLight == l)){
            selectedObject = nullptr;
            selectedLight = l;

            //Resetando os buffer de seleção
            strcpy(name_buffer, "\0");

            pos_buffer[0] = INFINITY; pos_buffer[1] = INFINITY; pos_buffer[2] = INFINITY;
            LIGHT_COLOR_RGB[0] = -1;
        }

        ++idx;
    }

    ImGui::EndChild();
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
        
        if (fabs(pos_buffer[0] - objPosition.getX()) > 0.001f || fabs(pos_buffer[1] - objPosition.getY()) > 0.001f || fabs(pos_buffer[2] - objPosition.getZ()) > 0.001f){ //Posição atualizada
            Vector3 newPos = Vector3(pos_buffer[0], pos_buffer[1], pos_buffer[2]);
            Vector3 dir = newPos - objPosition;
    
            selectedObject->move(dir);
        }

        ImGui::Text("Position");
        ImGui::SameLine();
        ImGui::InputFloat3("", pos_buffer);

        if (selectedObject->type == ObjType::TMesh){
            //Rotations
            if (ImGui::Button("-##1")) {
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->rotateX(-5.0f);
                }
            } 
            ImGui::SameLine();
            ImGui::Text("RotX");
            ImGui::SameLine(); 
            if (ImGui::Button("+##1")) {
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->rotateX(5.0f);
                }
            }

            if (ImGui::Button("-##2")) {
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->rotateY(-5.0f);
                }
            } 
            ImGui::SameLine();
            ImGui::Text("RotY");
            ImGui::SameLine(); 
            if (ImGui::Button("+##2")) {
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->rotateY(5.0f);
                }
            }

            if (ImGui::Button("-##3")) {
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->rotateZ(-5.0f);
                }
            } 
            ImGui::SameLine();
            ImGui::Text("RotZ");
            ImGui::SameLine(); 
            if (ImGui::Button("+##3")) {
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->rotateZ(5.0f);
                }
            }

            //Mirror
            if (ImGui::Button("MirrorX")){
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->mirrorX();
                }
            }

            if (ImGui::Button("MirrorY")){
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->mirrorY();
                }
            }

            if (ImGui::Button("MirrorZ")){
                if (selectedObject->type == ObjType::TMesh){
                    Mesh* m = (Mesh*) selectedObject;
                    m->mirrorZ();
                }
            }
        }

        //Remove Button
        if (ImGui::Button("Remove")){
            scene->removeObject(selectedObject);
            selectedObject = nullptr;
        }

        ImGui::End();
    }
    //

    //Selected Light
    if (selectedLight != nullptr){
        ImVec2 windowSize = ImVec2(300, 200);
        ImVec2 windowPos = ImVec2(screenSize.x - windowSize.x - 10, screenSize.y - windowSize.y - 10); // 10px padding

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
        ImGui::Begin("Selected");

        //Name field
        if (strcmp(name_buffer, "\0") == 0){
            strcpy(name_buffer, selectedLight->getName().c_str());
        }

        if (strcmp(name_buffer, selectedLight->getName().c_str()) != 0){ //Nome foi atualizado no input box
            selectedLight->setName(name_buffer);
        }

        ImGui::Text("Name"); 
        ImGui::SameLine();        
        ImGui::InputText("##hidden", name_buffer, IM_ARRAYSIZE(name_buffer));

        //Position field
        Vector3 lPosition = selectedLight->getPosition();

        if (pos_buffer[0] == INFINITY){ //não foi inicializado ainda
            pos_buffer[0] = lPosition.getX();
            pos_buffer[1] = lPosition.getY();
            pos_buffer[2] = lPosition.getZ();
        }
        
        if (fabs(pos_buffer[0] - lPosition.getX()) > 0.001f || fabs(pos_buffer[1] - lPosition.getY()) > 0.001f || fabs(pos_buffer[2] - lPosition.getZ()) > 0.001f){ //Posição atualizada
            Vector3 newPos = Vector3(pos_buffer[0], pos_buffer[1], pos_buffer[2]);
            Vector3 dir = newPos - lPosition;
    
            selectedLight->move(dir);
        }

        ImGui::Text("Position");
        ImGui::SameLine();
        ImGui::InputFloat3("", pos_buffer);
        
        //cor não inicializada para luz atual
        if (LIGHT_COLOR_RGB[0] == -1){
            LIGHT_COLOR_RGB[0] = selectedLight->colorRGB.getR();
            LIGHT_COLOR_RGB[1] = selectedLight->colorRGB.getG();
            LIGHT_COLOR_RGB[2] = selectedLight->colorRGB.getB();
        }

        if (ImGui::ColorEdit3("Color", LIGHT_COLOR_RGB)){
            selectedLight->setColor(Vector3(LIGHT_COLOR_RGB[0], LIGHT_COLOR_RGB[1], LIGHT_COLOR_RGB[2]));
        }

        ImGui::Checkbox("On", &selectedLight->getOn_ref());

        ImGui::Text((std::string("Type: ") + selectedLight->typeAsString()).c_str());

        if (selectedLight->getType() == LightType::Point){
            ImGui::Text("Potency");
            ImGui::SameLine();
            ImGui::InputFloat("##", &selectedLight->getPotency_ref());
        }
        else if (selectedLight->getType() == LightType::Spot){
            float& angle = selectedLight->getSpotAngle_ref();
            ImGui::Text("Range");
            ImGui::SameLine();
            ImGui::InputFloat("##1", &angle);

            if (angle <= 0.0f) angle = 0.01f;
            else if (angle >= 90.0f) angle = 89.99f;

            float& focus = selectedLight->getFocus_ref();
            ImGui::Text("Focus");
            ImGui::SameLine();
            ImGui::InputFloat("##2", &focus);
        }

        //Remove Button
        if (ImGui::Button("Remove")){
            scene->removeObject(selectedObject);
            selectedObject = nullptr;
        }

        ImGui::End();     
    }
    //

    char title[32];
    sprintf(title, "Fps: %.1f", 1.0/DELTA_TIME);
    SDL_SetWindowTitle(window, title);

    ImGui::Render();
        
    SDL_SetRenderDrawColor(renderer, 52, 52, 52, 255);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void Engine::__input_step(){
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse) return;

    Frame& f = scene->getCamera().getFrame();

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

    scene->getCamera().move(
        (f.getFrontDir() * velFRONT + f.getRightDir() * velRIGHT + f.getUpDir() * velUP) * speed * DELTA_TIME
    );
}

void Engine::__render_step(){
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;
    std::chrono::duration<double> duration;

    start = std::chrono::high_resolution_clock::now();
    scene->renderScene();

    SDL_GL_SwapWindow(Engine::getSceneWindow());
    end = std::chrono::high_resolution_clock::now();

    duration = end - start;
    DELTA_TIME = duration.count();
}

void Engine::__engine_loop(){
    SDL_Event event;

    while (Engine::onLoop){
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            Engine::__event_step(event);
        }

        if (DRAWING){
            SDL_GL_SwapWindow(Engine::getSceneWindow());
            Engine::__interface_step();
        }
        else{
            Engine::__interface_step();
            Engine::__input_step();
            Engine::__render_step();
        }

        SDL_Delay(16);
    }
}

SDL_Window* Engine::getConfigWindow() {return Engine::config_window;}
SDL_Window* Engine::getSceneWindow() {return Engine::scene_window;}

void Engine::Start(Scene& s, int scene_width, int scene_height, int config_width, int config_height){
    Engine::scene = &s;
    
    //Inicializando sdl
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        throw std::exception(); 
    }

    Engine::__init_ConfigWindow(s, config_width, config_height);
    Engine::__init_SceneWindow(s, scene_width, scene_height);

    scene->changeRenderizationMode(RenderizationMode::Wireframe);
    scene->switchOptimizedRenderization();
    scene->changeTextureFiltering(TextureFiltering::Bilinear);

    Engine::onLoop = true;
    Engine::__engine_loop();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    SDL_DestroyRenderer(SDL_GetRenderer(Engine::getSceneWindow()));
    SDL_DestroyWindow(Engine::getSceneWindow());

    SDL_GL_DeleteContext(SDL_GL_GetCurrentContext());
    SDL_DestroyWindow(Engine::getConfigWindow());

    SDL_Quit();
}
