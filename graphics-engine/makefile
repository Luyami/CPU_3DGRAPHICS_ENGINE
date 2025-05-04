objects = scene.o compiledlibs/datatypes.o compiledlibs/obj_importer.o compiledlibs/objects.o compiledlibs/raycast.o

all:
	g++ -c libsrc/obj_importer.cpp -o compiledlibs/obj_importer.o
	g++ -c libsrc/datatypes.cpp -o compiledlibs/datatypes.o
	g++ -fopenmp -c libsrc/objects.cpp -o compiledlibs/objects.o
	g++ -c libsrc/raycast.cpp -o compiledlibs/raycast.o
	g++ -c libsrc/engine.cpp -o compiledlibs/engine.o -I"../sdl/sdl/include"
	ar rcs compiledlibs/libmainGE.a compiledlibs/obj_importer.o compiledlibs/objects.o compiledlibs/datatypes.o compiledlibs/raycast.o compiledlibs/engine.o
	g++ -fopenmp -o scene scene.cpp ../imgui/src/*.cpp -I"../imgui/include" -I"../sdl/sdl/include" -L"compiledlibs" -L"../sdlbuild/Release" -L"../glewbuild/lib/Release" -pthread -lmainGE -lSDL2 -lglew32 -lopengl32 -lstdc++

scene:
	g++ -fopenmp -o scene scene.cpp ../imgui/src/*.cpp -I"../imgui/include" -I"../sdl/sdl/include" -L"compiledlibs" -L"../sdlbuild/Release" -L"../glewbuild/lib/Release" -pthread -lmainGE -lSDL2 -lglew32 -lopengl32 -lstdc++
