# Graficos por ordenador
El proyecto completo está en otro repo, hacer git clone --recursive https://github.com/juanpebm/gpo-framework.git 
Borrar todas las clases dentro de src menos GPO_aux.cpp

Modificar CMakeLists.txt y GPO-FRAMEWORK>.vscode>c_cpp_properties.json
Importar en bin>data todos los archivos necesarios

Crear clase parchis_inicial.cpp, Modelo3D.cpp, Modelo3D.h, starwars.cpp y copiar el contenido de este repo
incluir como primera linea de include>GpO.h : #pragma once
Ejecutar los comandos:
- Para construir el proyecto: cmake -DCMAKE_POLICY_VERSION_MINIMUM="3.5" ..
- Para compilar: cmake --build . --config Debug

Ya está, ahora tendremos que buscar el ejecutable del parchís en bin\Debug e ir compilando cada vez que cambiemos código.
****************************
Contenido de CMakeLists.txt:
****************************

cmake_minimum_required ( VERSION 3.7 )
project ( GpO VERSION 0.0.2)
set( GPO_VERSION_ABI 1 )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC ")
if (MSVC)
    add_compile_options(/std:c++17)
else()
    add_compile_options(-std=c++17 -fPIC)
endif()

if( "${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}" )
 message( FATAL "no in source building allowed." )
endif( )

list( APPEND CMAKE_MODULE_PATH
 ${PROJECT_SOURCE_DIR}/CMake
)

set( GPO_DESCRIPTION "GpO Base Project" )
set( GPO_LICENSE LGPL )

set(BUILD_SHARED_LIBS OFF)

find_package(OpenGL REQUIRED)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_subdirectory(libs/glm )

add_subdirectory(libs/glfw EXCLUDE_FROM_ALL)
add_subdirectory(libs/glad )

set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_ZLIB ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)
add_subdirectory(libs/assimp )

include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} )
include_directories(libs/glad/include)
include_directories(libs/glm)
include_directories(libs/imgui)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/KHR)
include_directories(${PROJECT_SOURCE_DIR}/include/stb)

set(GPO_SOURCE_FILES include/stb/stb_image.h
                     include/GpO.h
                     src/parchis_inicial.cpp
                     src/Modelo3D.cpp
                     src/GPO_aux.cpp
                     libs/imgui/imgui.cpp
                     libs/imgui/imgui_draw.cpp
                     libs/imgui/imgui_tables.cpp
                     libs/imgui/imgui_widgets.cpp
                     libs/imgui/backends/imgui_impl_glfw.cpp
                     libs/imgui/backends/imgui_impl_opengl3.cpp)
add_executable(parchis_inicial ${GPO_SOURCE_FILES})
include_directories(libs/assimp/include)
include_directories(libs/imgui/backends)
target_link_libraries(parchis_inicial PRIVATE ${OPENGL_LIBRARIES} glfw glm glad_interface assimp)

set(GPO_SOURCE_FILES include/stb/stb_image.h
					 include/GpO.h
					 src/starwars.cpp
					 src/Modelo3D.cpp
					 src/GPO_aux.cpp)
add_executable(starwars ${GPO_SOURCE_FILES})
target_link_libraries(starwars PRIVATE ${OPENGL_LIBRARIES} glfw glm glad_interface assimp)


set_target_properties(parchis_inicial
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

  set_target_properties(starwars
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)

#GUI interface
set(GPO_GUI_SOURCE_FILES  	libs/imgui/imgui.cpp
							libs/imgui/imgui_demo.cpp
							libs/imgui/imgui_draw.cpp
							libs/imgui/imgui_tables.cpp
							libs/imgui/imgui_widgets.cpp
							libs/imgui/backends/imgui_impl_glfw.cpp
							libs/imgui/backends/imgui_impl_opengl3.cpp
							tests/gui/min_gui.cpp)

add_executable(gpo_min_gui ${GPO_GUI_SOURCE_FILES})
target_link_libraries(gpo_min_gui PRIVATE ${OPENGL_LIBRARIES} glfw glm glad_interface)

set_target_properties(gpo_min_gui
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#Import
set(GPO_ASS_SOURCE_FILES  	include/stb/stb_image.h
							include/GpO.h
							src/GPO_aux.cpp
							tests/assimp/GPO_assimp_aux.h
							tests/assimp/GPO_assimp_aux.cpp
							tests/assimp/min_assimp.cpp
							)
add_executable(gpo_min_assimp ${GPO_ASS_SOURCE_FILES})
target_link_libraries(gpo_min_assimp PRIVATE ${OPENGL_LIBRARIES} glfw glm glad_interface assimp)
set_target_properties(gpo_min_assimp
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

file(GLOB MY_CONTENT "data/*")
file(COPY ${MY_CONTENT} DESTINATION "${CMAKE_BINARY_DIR}/bin/data")


****************************
Contenido de GPO-FRAMEWORK>.vscode>c_cpp_properties.json  :
****************************

{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include",
                "${workspaceFolder}/include/stb",
                "${workspaceFolder}/libs/assimp/include",
                "${workspaceFolder}/libs/glfw/include",
                "${workspaceFolder}/libs/glm"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.26100.0",
            "compilerPath": "cl.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-msvc-x64"
        }
    ],
    "version": 4
}

****************************
Contenido de Modelo3D.cpp:
****************************

#include "Modelo3D.h"

#define GLSL(src) "#version 330 core\n" #src

// ===================== SHADERS =====================

static const char* vertex_prog = GLSL(
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec3 normal;

uniform mat4 MVP;
uniform mat4 M;

out vec3 vColor;
out vec3 vNormal;

void main() {
    gl_Position = MVP * vec4(pos, 1.0);
    vColor = col;
    vNormal = mat3(transpose(inverse(M))) * normal;
}
);

static const char* fragment_prog = GLSL(
in vec3 vColor;
in vec3 vNormal;

out vec4 color;

void main() {

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.8));

    vec3 N = normalize(vNormal);

    float diffuse = max(dot(N, lightDir), 0.2);

    vec3 finalColor = vColor * diffuse;

    color = vec4(finalColor, 1.0);
}
);

// ===================== CONSTRUCTOR =====================

Modelo3D::Modelo3D(const char* path) {
    cargarModelo(path);
    crearShaders();
}

// ===================== CARGA MODELO =====================

void Modelo3D::cargarModelo(const char* path) {

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_PreTransformVertices
    );

    if (!scene || !scene->HasMeshes()) {
        std::cout << "Error cargando modelo: "
                  << importer.GetErrorString() << std::endl;
        exit(-1);
    }

    std::vector<float> data;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {

        aiMesh* mesh = scene->mMeshes[m];

        aiColor3D diffuse(1.0f, 1.0f, 1.0f);

        if (scene->HasMaterials()) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {

            aiFace face = mesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; j++) {

                unsigned int idx = face.mIndices[j];

                aiVector3D v = mesh->mVertices[idx];
                aiVector3D n = mesh->mNormals[idx];

                data.push_back(v.x);
                data.push_back(v.y);
                data.push_back(v.z);

                data.push_back(n.x);
                data.push_back(n.y);
                data.push_back(n.z);

                data.push_back(diffuse.r);
                data.push_back(diffuse.g);
                data.push_back(diffuse.b);
            }
        }
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        data.size() * sizeof(float),
        data.data(),
        GL_STATIC_DRAW
    );

    // Posición
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        9 * sizeof(float),
        (void*)0
    );

    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        9 * sizeof(float),
        (void*)(6 * sizeof(float))
    );

    // Normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        3,
        GL_FLOAT,
        GL_FALSE,
        9 * sizeof(float),
        (void*)(3 * sizeof(float))
    );

    glBindVertexArray(0);

    numVertices = data.size() / 9;
}

// ===================== SHADERS =====================

void Modelo3D::crearShaders() {

    GLuint vs = compilar_shader(vertex_prog, GL_VERTEX_SHADER);
    GLuint fs = compilar_shader(fragment_prog, GL_FRAGMENT_SHADER);

    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);

    glLinkProgram(shaderProgram);

    check_errores_programa(shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

// ===================== DRAW =====================

void Modelo3D::draw(mat4 P, mat4 V, mat4 M) {

    glUseProgram(shaderProgram);
    
    transfer_mat4("MVP", P * V * M);
    transfer_mat4("M", M);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
    glBindVertexArray(0);
}

****************************
Contenido de Modelo3D.h:
****************************

#pragma once

#include <GpO.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <string>
#include <iostream>

class Modelo3D {
private:
    GLuint VAO, VBO;
    GLuint shaderProgram;
    int numVertices;

    void cargarModelo(const char* path);
    void crearShaders();

public:
    Modelo3D(const char* path);

    void draw(mat4 P, mat4 V, mat4 M);

    GLuint getVAO() const { return VAO; }
};


****************************
Contenido de starwars.cpp:
****************************

#include <GpO.h>
#include "Modelo3D.h"

#include <iostream>

int ANCHO = 800, ALTO = 600;
GLFWwindow* window;

float angulo_h = 0.0f;
float angulo_v = 0.3f;
float radio    = 1.0f;

bool rightMouseDown = false;
double lastX, lastY;

Modelo3D* modelo = nullptr;

// ===================== RATÓN =====================

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rightMouseDown = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE) {
            rightMouseDown = false;
        }
    }
}

void mouse_orbital(GLFWwindow* window, double xpos, double ypos)
{
    if (!rightMouseDown) return;

    float dx = xpos - lastX;
    float dy = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    float sens = 0.005f;

    angulo_h -= dx * sens;
    angulo_v += dy * sens;

    if (angulo_v < -1.2f) angulo_v = -1.2f;
    if (angulo_v >  1.2f) angulo_v =  1.2f;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    radio -= yoffset * 0.06f;

    if (radio > 2.0f) radio = 2.0f;
    if (radio < 0.005f) radio = 0.005f;
}

// ===================== INIT =====================

void init_scene()
{
    glEnable(GL_DEPTH_TEST);

    glfwSetCursorPosCallback(window, mouse_orbital);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

// ===================== RENDER =====================

void render_scene()
{
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 P = perspective(
        glm::radians(45.0f),
        float(ANCHO) / float(ALTO),
        0.01f,
        100.0f
    );

    vec3 cam(
        radio * cos(angulo_h) * cos(angulo_v),
        radio * sin(angulo_h) * cos(angulo_v),
        radio * sin(angulo_v)
    );

    mat4 V = lookAt(
        cam,
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f)
    );

    mat4 M = mat4(1.0f);
    M = scale(M, vec3(0.01f));
    M = rotate(M, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    modelo->draw(P, V, M);
}

// ===================== CALLBACKS WINDOW =====================

void ResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    ANCHO = width;
    ALTO = height;
}

void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, true);
}

void asigna_funciones_callback(GLFWwindow* window)
{
    glfwSetWindowSizeCallback(window, ResizeCallback);
    glfwSetKeyCallback(window, KeyCallback);
}

// ===================== MAIN =====================

int main()
{
    init_GLFW();

    window = Init_Window("Star Wars Model Viewer");

    load_Opengl();
    modelo = new Modelo3D("../data/Mask.obj");

    asigna_funciones_callback(window);

    init_scene();

    while (!glfwWindowShouldClose(window))
    {
        render_scene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


****************************
Contenido de parchis_inicial.cpp:
****************************

#include <GpO.h>
#include "Modelo3D.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


int ANCHO = 800, ALTO = 600;
const char* prac = "Parchis Inicial";


Modelo3D* Hangar = nullptr;
Modelo3D* nave = nullptr;
Modelo3D* TieInterceptor = nullptr;
Modelo3D* R2D2 = nullptr;
Modelo3D* Mask = nullptr;

GLFWwindow* window;
GLuint prog;
objeto tablero_superior;
objeto tablero_inferior;
objeto tablero_laterales;
objeto zonas_color;
objeto casas;
objeto pasillos;
objeto estrella;
objeto bordes;



//movimiento camara variables
float radio  = 4.0f;     // distancia al centro

float angulo_h = 0.0f;   // giro horizontal
float angulo_v = 0.3f;   // giro vertical

float min_v = 0.15f;     // límite inferior
float max_v = 1.2f;      // límite superior
static double lastX, lastY;

vec3 pos_obs = vec3(0.0f, 0.0f, 3.0f);
bool rightMouseDown = false;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


// LOGICA
enum ColorJugador {
    SIN_COLOR = -1,
    ROJO = 0,
    AZUL = 1,
    VERDE = 2,
    AMARILLO = 3
};

enum TipoCasilla {
    NORMAL = 0,
    SEGURO = 1,
    SALIDA = 2,
    META = 3,        // camino de meta
    FINAL = 4,       // última casilla (triángulo)
    CASA = 5         // base inicial (las 4 de cada color)
};

struct Casilla {
    int id;                 // índice en el vector
    float x, y;             // posición lógica para dibujar
    int next;               // siguiente en el camino normal (-1 si no aplica)
    int next_meta;          // siguiente en el camino de meta (-1 si no aplica)
    TipoCasilla tipo;
    ColorJugador color;     // dueño si aplica (salida, meta, final, casa, etc.)
};
std::vector<Casilla> grafo;

struct Ficha {
    int casilla_actual;   // índice en el grafo
    ColorJugador color;
    bool en_meta;      
};

Ficha fichas[16];          // 4 fichas amarillas de momento
int ficha_seleccionada = 0;
float tiempo_ultimo_movimiento = 0.0f;
float intervalo_movimiento = 1.0f;

int turno_actual = 0;  // 0=AMARILLO, 1=AZUL, 2=ROJO, 3=VERDE
ColorJugador orden_turnos[4] = { AMARILLO, AZUL, ROJO, VERDE };
int dado = 0;          // resultado del dado
bool dado_lanzado = false;

double tiempo_anterior = 0.0;

// ======================= SHADERS =========================

#define GLSL(src) "#version 330 core\n" #src

const char* vertex_prog = GLSL(
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
out vec3 col;
uniform mat4 MVP = mat4(1.0f);
void main() 
{
    gl_Position = MVP * vec4(pos, 1);
    col = color;
}
);

const char* fragment_prog = GLSL(
in vec3 col;
out vec3 outputColor;
void main() 
{
    outputColor = col;
}
);

// ======================= CREAR RECTANGULO en un solo array =========================

void addRect(float* verts, float* cols, int& idx,
             float x0, float y0, float x1, float y1,
             float z, float r, float g, float b)
{
    float v[18] = {
        x0,y0,z,  x1,y0,z,  x1,y1,z,
        x0,y0,z,  x1,y1,z,  x0,y1,z
    };

    float c[18] = {
        r,g,b, r,g,b, r,g,b,
        r,g,b, r,g,b, r,g,b
    };

    for(int i=0;i<18;i++){
        verts[idx*18 + i] = v[i];
        cols[idx*18 + i]  = c[i];
    }

    idx++;
}

objeto crear_tablero_superior_base()
{
    objeto obj;
    GLuint VAO;

    float z = 0.00f;

    float verts[] = {
        // triángulo 1
        -1.0f, -1.0f, z,
         1.0f, -1.0f, z,
         1.0f,  1.0f, z,

        // triángulo 2
        -1.0f, -1.0f, z,
         1.0f,  1.0f, z,
        -1.0f,  1.0f, z
    };

    float col[] = {
        // blanco suave
        0.95f, 0.95f, 0.95f,
        0.95f, 0.95f, 0.95f,
        0.95f, 0.95f, 0.95f,
        0.95f, 0.95f, 0.95f,
        0.95f, 0.95f, 0.95f,
        0.95f, 0.95f, 0.95f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &colbuf);
    glBindBuffer(GL_ARRAY_BUFFER, colbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(col), col, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 6;
    return obj;
}
objeto crear_tablero_inferior()
{
    objeto obj;
    GLuint VAO, VBO;

    float z = -0.1f; // grosor

    float verts[] = {
        // triángulo 1
        -1.0f, -1.0f, z,
         1.0f, -1.0f, z,
         1.0f,  1.0f, z,

        // triángulo 2
        -1.0f, -1.0f, z,
         1.0f,  1.0f, z,
        -1.0f,  1.0f, z
    };

    float col[] = {
        // color madera oscuro
        0.3f, 0.2f, 0.1f,
        0.3f, 0.2f, 0.1f,
        0.3f, 0.2f, 0.1f,
        0.3f, 0.2f, 0.1f,
        0.3f, 0.2f, 0.1f,
        0.3f, 0.2f, 0.1f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &colbuf);
    glBindBuffer(GL_ARRAY_BUFFER, colbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(col), col, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 6;
    return obj;
}
objeto crear_tablero_laterales()
{
    objeto obj;
    GLuint VAO;

    float z0 = 0.0f;
    float z1 = -0.1f;

    float verts[] = {
        // Lado norte
        -1.0f,  1.0f, z0,   1.0f,  1.0f, z0,   1.0f,  1.0f, z1,
        -1.0f,  1.0f, z0,   1.0f,  1.0f, z1,  -1.0f,  1.0f, z1,

        // Lado sur
        -1.0f, -1.0f, z0,   1.0f, -1.0f, z0,   1.0f, -1.0f, z1,
        -1.0f, -1.0f, z0,   1.0f, -1.0f, z1,  -1.0f, -1.0f, z1,

        // Lado este
         1.0f, -1.0f, z0,   1.0f,  1.0f, z0,   1.0f,  1.0f, z1,
         1.0f, -1.0f, z0,   1.0f,  1.0f, z1,   1.0f, -1.0f, z1,

        // Lado oeste
        -1.0f, -1.0f, z0,  -1.0f,  1.0f, z0,  -1.0f,  1.0f, z1,
        -1.0f, -1.0f, z0,  -1.0f,  1.0f, z1,  -1.0f, -1.0f, z1
    };

    float col[6 * 4 * 3];
    for (int i = 0; i < 6 * 4; i++) {
        col[i*3+0] = 0.4f;
        col[i*3+1] = 0.25f;
        col[i*3+2] = 0.15f;
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &colbuf);
    glBindBuffer(GL_ARRAY_BUFFER, colbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(col), col, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 6 * 4;
    return obj;
}
objeto crear_zonas_color()
{
    objeto obj;
    GLuint VAO;

    float z = 0.001f;
    float d = 0.4f; //distancia entre el medio y cada uno de los 4 rectangulos (si es 0.2 los rect seran mas grandes)

    // 4 rectángulos → 4 * 18 floats = 72 floats
    float verts[72];
    float cols[72];

    int idx = 0;

    // Colores según tu foto
    float azul[3]     = {0.2f, 0.2f, 0.9f};
    float amarillo[3] = {0.9f, 0.9f, 0.2f};
    float verde[3]    = {0.2f, 0.8f, 0.2f};
    float rojo[3]     = {0.9f, 0.2f, 0.2f};

    // Zonas
    addRect(verts, cols, idx, -1.0f,  d, -d,  1.0f, z, azul[0], azul[1], azul[2]);     // arriba izda
    addRect(verts, cols, idx,  d,  d,  1.0f,  1.0f, z, amarillo[0], amarillo[1], amarillo[2]); // arriba dcha
    addRect(verts, cols, idx,  d, -1.0f,  1.0f, -d, z, verde[0], verde[1], verde[2]);    // abajo dcha
    addRect(verts, cols, idx, -1.0f, -1.0f, -d, -d, z, rojo[0], rojo[1], rojo[2]);     // abajo izda


    // Crear VAO único
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, col;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &col);
    glBindBuffer(GL_ARRAY_BUFFER, col);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cols), cols, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv  = 24; // 4 rectángulos * 6 vértices

    return obj;
}

objeto crear_pasillos()
{
    objeto obj;
    GLuint VAO;

    float z = 0.003f; // un poco por encima de casas
    float w = 0.2f;   // ancho del pasillo

    float verts[] = {
        // AZUL (arriba, centro) 
        -w/2, 0.0f, z,   w/2, 0.0f, z,   w/2, 1.0f, z,
        -w/2, 0.0f, z,   w/2, 1.0f, z,  -w/2, 1.0f, z,

        // AMARILLO (derecha, centro) 
         0.0f, -w/2, z,   1.0f, -w/2, z,   1.0f, w/2, z,
         0.0f, -w/2, z,   1.0f, w/2, z,    0.0f, w/2, z,

        // VERDE (abajo, centro) 
        -w/2, -1.0f, z,   w/2, -1.0f, z,   w/2, 0.0f, z,
        -w/2, -1.0f, z,   w/2, 0.0f, z,   -w/2, 0.0f, z,

        // ROJO (izquierda, centro) 
        -1.0f, -w/2, z,   0.0f, -w/2, z,   0.0f, w/2, z,
        -1.0f, -w/2, z,   0.0f, w/2, z,   -1.0f, w/2, z
    };

    float col[] = {
        // AZUL
        0.2f,0.2f,0.9f,  0.2f,0.2f,0.9f,  0.2f,0.2f,0.9f,
        0.2f,0.2f,0.9f,  0.2f,0.2f,0.9f,  0.2f,0.2f,0.9f,

        // AMARILLO
        0.9f,0.9f,0.2f,  0.9f,0.9f,0.2f,  0.9f,0.9f,0.2f,
        0.9f,0.9f,0.2f,  0.9f,0.9f,0.2f,  0.9f,0.9f,0.2f,

        // VERDE
        0.2f,0.8f,0.2f,  0.2f,0.8f,0.2f,  0.2f,0.8f,0.2f,
        0.2f,0.8f,0.2f,  0.2f,0.8f,0.2f,  0.2f,0.8f,0.2f,

        // ROJO
        0.9f,0.2f,0.2f,  0.9f,0.2f,0.2f,  0.9f,0.2f,0.2f,
        0.9f,0.2f,0.2f,  0.9f,0.2f,0.2f,  0.9f,0.2f,0.2f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &colbuf);
    glBindBuffer(GL_ARRAY_BUFFER, colbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(col), col, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 6 * 4;
    return obj;
}
objeto crear_estrella_central()
{
    objeto obj;
    GLuint VAO;

    float z = 0.004f;   // un poco por encima de los pasillos
    float r = 0.3f;     // tamaño de la estrella

    float verts[] = {
        0.0f, 0.0f, z,   -r,  r, z,    r,  r, z,
        0.0f, 0.0f, z,    r,  r, z,    r, -r, z,
        0.0f, 0.0f, z,   -r, -r, z,    r, -r, z,
        0.0f, 0.0f, z,   -r,  r, z,   -r, -r, z
    };

    float col[] = {
        // AZUL
        0.2f,0.2f,0.9f,  0.2f,0.2f,0.9f,  0.2f,0.2f,0.9f,
        // AMARILLO
        0.9f,0.9f,0.2f,  0.9f,0.9f,0.2f,  0.9f,0.9f,0.2f,
        // VERDE
        0.2f,0.8f,0.2f,  0.2f,0.8f,0.2f,  0.2f,0.8f,0.2f,
        // ROJO
        0.9f,0.2f,0.2f,  0.9f,0.2f,0.2f,  0.9f,0.2f,0.2f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &colbuf);
    glBindBuffer(GL_ARRAY_BUFFER, colbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(col), col, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 12; // 4 triángulos
    return obj;
}
objeto crear_bordes()
{
    objeto obj;
    GLuint VAO;

    float z_top = 0.01f;  // un poco por encima de la tapa
    float z_bottom = -0.03f;
    float t = 0.06f;   // grosor del marco
    float o = 0.02f;   // ← DESPLAZAMIENTO HACIA FUERA SOLO PARA LAS CARAS VERTICALES

    float verts[] = {

        // ─────────────────────────────
        // BORDE SUPERIOR (horizontal)  le sumamos o, para que tape el agujero que deja el desplazamiento de la cara vertical
        // ─────────────────────────────
        1.0f + o,  1.0f + o, z_top,    -1.0f - o,  1.0f + o, z_top,   1.0f + o,  1.0f - t + o, z_top,
        1.0f + o,  1.0f - t + o, z_top,   -1.0f + o,  1.0f + o, z_top,   -1.0f - o,  1.0f - t + o, z_top,

        // Cara vertical superior  ← SOLO ESTA SE MUEVE
        -1.0f - o,  1.0f + o, z_top,   1.0f + o,  1.0f + o, z_top,     1.0f + o,  1.0f + o, z_bottom,
        -1.0f - o,  1.0f + o, z_top,   1.0f + o,  1.0f + o, z_bottom, -1.0f - o,  1.0f + o, z_bottom,


        // ─────────────────────────────
        // BORDE INFERIOR (horizontal)
        // ─────────────────────────────
        1.0f + o, -1.0f + t - o, z_top,   -1.0f - o, -1.0f + t - o, z_top,   1.0f + o, -1.0f - o, z_top,
        1.0f + o, -1.0f - o, z_top,     -1.0f - o, -1.0f + t - o, z_top,   -1.0f - o, -1.0f - o, z_top,

        // Cara vertical inferior  ← SOLO ESTA SE MUEVE
        -1.0f - o, -1.0f - o, z_top,   1.0f + o, -1.0f - o, z_top,   1.0f + o, -1.0f - o, z_bottom,
        -1.0f - o, -1.0f - o, z_top,   1.0f + o, -1.0f - o, z_bottom,  -1.0f - o, -1.0f - o, z_bottom,


        // ─────────────────────────────
        // BORDE IZQUIERDO (horizontal)
        // ─────────────────────────────
        -1.0f - o, -1.0f - o, z_top,   -1.0f + t - o, -1.0f - o, z_top,   -1.0f + t - o, 1.0f + o, z_top,
        -1.0f - o, -1.0f - o, z_top,   -1.0f + t - o, 1.0f + o, z_top,    -1.0f - o, 1.0f + o, z_top,

        // Cara vertical izquierda  ← SOLO ESTA SE MUEVE
        -1.0f - o, -1.0f - o, z_top,   -1.0f - o,  1.0f + o, z_top,   -1.0f - o,  1.0f + o, z_bottom,
        -1.0f - o, -1.0f - o, z_top,   -1.0f - o,  1.0f + o, z_bottom,  -1.0f - o, -1.0f - o, z_bottom,


        // ─────────────────────────────
        // BORDE DERECHO (horizontal)
        // ─────────────────────────────
         1.0f - t + o, -1.0f - o, z_top,   1.0f + o, -1.0f - o, z_top,   1.0f + o, 1.0f + o, z_top,
         1.0f - t + o, -1.0f - o, z_top,   1.0f + o, 1.0f + o, z_top,    1.0f - t + o, 1.0f + o, z_top,

        // Cara vertical derecha  ← SOLO ESTA SE MUEVE
         1.0f + o, -1.0f - o, z_top,   1.0f + o,  1.0f + o, z_top,   1.0f + o,  1.0f + o, z_bottom,
         1.0f + o, -1.0f - o, z_top,   1.0f + o,  1.0f + o, z_bottom,  1.0f + o, -1.0f - o, z_bottom
    };

    float col[6 * 8 * 3];
    for (int i = 0; i < 6 * 8; i++) {
        col[i*3+0] = 0.0f;
        col[i*3+1] = 0.0f;
        col[i*3+2] = 0.0f;
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &colbuf);
    glBindBuffer(GL_ARRAY_BUFFER, colbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(col), col, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 6 * 8;
    return obj;
}

//logica solo este obj
objeto crear_casillas_desde_grafo(float size, float z) {
    objeto obj;
    GLuint VAO;
    
    int MAX = grafo.size();
    std::vector<float> verts;
    std::vector<float> cols;
    verts.reserve(MAX * 6 * 3);
    cols.reserve(MAX * 6 * 3);

    auto push_rect = [&](float cx, float cy, float s, float r, float g, float b) {
        float x0 = cx - s*0.5f;
        float x1 = cx + s*0.5f;
        float y0 = cy - s*0.5f;
        float y1 = cy + s*0.5f;

        float v[18] = {
            x0,y0,z,  x1,y0,z,  x1,y1,z,
            x0,y0,z,  x1,y1,z,  x0,y1,z
        };
        float c[18] = {
            r,g,b, r,g,b, r,g,b,
            r,g,b, r,g,b, r,g,b
        };
        verts.insert(verts.end(), v, v+18);
        cols.insert(cols.end(),  c, c+18);
    };

    for (auto &c : grafo) {
        float r=0.9f,g=0.9f,b=0.9f;

        if (c.tipo == SEGURO) { r = g = b = 0.7f; }
        if (c.tipo == SALIDA) { r = 1.0f; g = 1.0f; b = 1.0f; }
        if (c.tipo == META || c.tipo == FINAL || c.tipo == CASA) {
            if (c.color == ROJO)     { r=0.65f; g=0.15f; b=0.15f; }
            if (c.color == AZUL)     { r=0.15f; g=0.15f; b=0.70f; }
            if (c.color == VERDE)    { r=0.15f; g=0.60f; b=0.15f; }
            if (c.color == AMARILLO) { r=0.70f; g=0.70f; b=0.15f; }

        }

        push_rect(c.x, c.y, size, r,g,b);
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, col;
    glGenBuffers(1, &pos);
    glBindBuffer(GL_ARRAY_BUFFER, pos);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

    glGenBuffers(1, &col);
    glBindBuffer(GL_ARRAY_BUFFER, col);
    glBufferData(GL_ARRAY_BUFFER, cols.size()*sizeof(float), cols.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv  = (int)verts.size()/3;
    return obj;
}


void mouse_orbital(GLFWwindow* window, double xpos, double ypos)
{
    if (!rightMouseDown)
        return;   // <-- NO ROTAR si no está pulsado el botón derecho

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sens = 0.005f;
    xoffset *= sens;
    yoffset *= sens;

    // actualizar ángulos
    angulo_h -= xoffset;
    angulo_v -= yoffset;

    // limitar ángulo vertical
    if (angulo_v < min_v) angulo_v = min_v;
    if (angulo_v > max_v) angulo_v = max_v;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    radio -= yoffset * 0.2f;
    if (radio < 2.0f) radio = 2.0f;   // límite mínimo
    if (radio > 7.0f) radio = 7.0f; // límite máximo
}


//LOGICA
void crear_grafo_parchis() {
    grafo.clear();
    grafo.resize(116); // 68 + 32 + 16

    for (int i = 0; i < (int)grafo.size(); ++i) {
        grafo[i].id = i;
        grafo[i].x = grafo[i].y = 0.0f;
        grafo[i].next = -1;
        grafo[i].next_meta = -1;
        grafo[i].tipo = NORMAL;
        grafo[i].color = SIN_COLOR;
    }
}
void configurar_camino_principal() {
    const int N = 68;
    for (int i = 0; i < N; ++i) {
        grafo[i].next = (i + 1) % N;
        grafo[i].tipo = NORMAL;
    }

    // Aquí marca casillas seguras, salidas, etc. según el reglamento
    // Casillas seguras típicas (en muchos tableros): 5, 12, 17, 22, 29, 34, 39, 46, 51, 56, 63, 68
    grafo[4].tipo  = SEGURO;
    grafo[11].tipo = SEGURO;
    grafo[16].tipo = SEGURO;
    grafo[21].tipo = SEGURO;
    grafo[28].tipo = SEGURO;
    grafo[33].tipo = SEGURO;
    grafo[38].tipo = SEGURO;
    grafo[45].tipo = SEGURO;
    grafo[50].tipo = SEGURO;
    grafo[55].tipo = SEGURO;
    grafo[62].tipo = SEGURO;  
    grafo[67].tipo = SEGURO;
}
struct InfoColor {
    ColorJugador color;
    int casilla_entrada_meta;   // índice en 0..67
    int primera_meta;           // índice en 68..91
};

void configurar_metas() {
    InfoColor info[4] = {
        { AMARILLO,  67,  68 }, // 68..75
        { AZUL,      16,  76 }, // 76..83
        { ROJO,      33,  84 }, // 84..91
        { VERDE,     50,  92 }  // 92..99
    };
        

    for (int c = 0; c < 4; ++c) {
        ColorJugador col = info[c].color;
        int entrada = info[c].casilla_entrada_meta;
        int base    = info[c].primera_meta;

        // La casilla de entrada del camino principal "salta" a la primera de meta
        grafo[entrada].color = col;
        grafo[entrada].next_meta = base;

        // Configuramos las 6 casillas de meta
        for (int k = 0; k < 8; ++k) {
            int id = base + k;
            grafo[id].tipo  = (k == 7 ? FINAL : META);
            grafo[id].color = col;
            grafo[id].next  = (k == 7 ? -1 : id + 1); // la última no tiene siguiente
        }
    }
}
void configurar_casas() {
    int base_amarillo = 100;
    int base_azul     = 104;
    int base_rojo     = 108;
    int base_verde    = 112;

    for (int i = 0; i < 4; ++i) {
        grafo[base_amarillo + i].tipo  = CASA;
        grafo[base_amarillo + i].color = AMARILLO;

        grafo[base_azul + i].tipo  = CASA;
        grafo[base_azul + i].color = AZUL;

        grafo[base_rojo + i].tipo  = CASA;
        grafo[base_rojo + i].color = ROJO;

        grafo[base_verde + i].tipo  = CASA;
        grafo[base_verde + i].color = VERDE;
    }
}
void asignar_posiciones_camino_principal() {
    float step = 0.08f;
    float indice = 0;
    
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  0.90f - step*i; grafo[indice].y = 0.25f; //0 al 7
        indice++;
    }
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  0.25f; grafo[indice].y = 0.34f + step * i; //8 al 15
        indice++;
    }
    grafo[indice].x = 0.00f; grafo[indice].y = 0.90f; //casilla de meta AZUL
    indice++;
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  -0.25f; grafo[indice].y = 0.90f - step * i; //17 al 24
        indice++;
    }
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  -0.34f - step*i; grafo[indice].y = 0.25f; //25 al 32
        indice++;
    }
    grafo[indice].x = -0.90f; grafo[indice].y = 0.00f; //casilla de meta ROJA
    indice++;
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  -0.90f + step*i; grafo[indice].y = -0.25f; //34 al 41
        indice++;
    }
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  -0.25f; grafo[indice].y = -0.34f - step * i; //42 al 49
        indice++;
    }
    grafo[indice].x = 0.00f; grafo[indice].y = -0.90f; //casilla de meta VERDE
    indice++;
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  0.25f; grafo[indice].y = -0.90f + step * i; //51 al 58
        indice++;
    }
    for(int i = 0; i < 8; ++i){
        grafo[indice].x =  0.34f + step*i; grafo[indice].y = -0.25f; //59 al 66
        indice++;
    }
    grafo[indice].x =  0.90f; grafo[indice].y = 0.00f;   // casilla segura de meta AMARILLO 
    indice++;
}
void asignar_posiciones_metas() {
    float step = 0.08f;

    // AMARILLO (derecha → centro)  id = 68..75
    for (int k = 0; k < 8; ++k) {
        int id = 68 + k;
        grafo[id].x = 0.90f - step * (k + 1);
        grafo[id].y = 0.0f;
    }

    // AZUL (arriba → centro) id = 76..83
    for (int k = 0; k < 8; ++k) {
        int id = 76 + k;
        grafo[id].x = 0.0f;
        grafo[id].y = 0.90f - step * (k + 1);
    }

    // ROJO (izquierda → centro) id = 84..91
    for (int k = 0; k < 8; ++k) {
        int id = 84 + k;
        grafo[id].x = -0.90f + step * (k + 1);
        grafo[id].y = 0.0f;
    }

    // VERDE (abajo → centro) id = 92..99
    for (int k = 0; k < 8; ++k) {
        int id = 92 + k;
        grafo[id].x = 0.0f;
        grafo[id].y = -0.90f + step * (k + 1);
    }
}

void asignar_posiciones_casas() {
    float d = 0.6f; //distancia desde el centro del tablero hasta el centro de cada base
    float s = 0.08f; //separacion entre casillas

    auto set = [&](int base, float cx, float cy) {
        grafo[base+0].x = cx-s; grafo[base+0].y = cy-s;
        grafo[base+1].x = cx+s; grafo[base+1].y = cy-s;
        grafo[base+2].x = cx-s; grafo[base+2].y = cy+s;
        grafo[base+3].x = cx+s; grafo[base+3].y = cy+s;
    };

    set(100,  d,  d);   // arriba drcha
    set(104, -d,  d);   // arriba izda
    set(108, -d, -d);   // abajo izda
    set(112,  d, -d);   // abajo drcha

}
void construir_grafo_y_posiciones() {
    crear_grafo_parchis();
    configurar_camino_principal();
    configurar_metas();
    configurar_casas();
    asignar_posiciones_camino_principal();
    asignar_posiciones_metas();
    asignar_posiciones_casas();
}


void mover_ficha(Ficha& f, int pasos) {
    for (int p = 0; p < pasos; p++) {
        int actual = f.casilla_actual;
        int siguiente = -1;

        if (actual >= 100) {
            if (pasos != 5) break;  // en parchís real solo sales con un 5
            if (f.color == AMARILLO) siguiente = 0;
            else if (f.color == AZUL)  siguiente = 17;
            else if (f.color == ROJO)  siguiente = 34;
            else if (f.color == VERDE) siguiente = 51;
        } else if (!f.en_meta && grafo[actual].next_meta != -1 
                   && f.color == grafo[actual].color) {
            siguiente = grafo[actual].next_meta;
            f.en_meta = true;
        } else {
            siguiente = grafo[actual].next;
        }

        if (siguiente == -1) break;
        f.casilla_actual = siguiente;
    }
}

// ======================= INIT SCENE =========================

void init_scene() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);

//importados
    Hangar = new Modelo3D("../data/hangar2.obj");
    nave = new Modelo3D("../data/nave.obj"); 
    TieInterceptor = new Modelo3D("../data/TieInterceptor.obj"); 
    R2D2 = new Modelo3D("../data/R2-D2.obj");  
    Mask = new Modelo3D("../data/Mask.obj");  

//logica
    construir_grafo_y_posiciones();
    casas = crear_casillas_desde_grafo(0.06f, 0.005f); // por ejemplo 0.06 de size y 0.005 de altura z


	tablero_superior = crear_tablero_superior_base();
	tablero_inferior = crear_tablero_inferior();
	tablero_laterales = crear_tablero_laterales();
	zonas_color = crear_zonas_color();
	pasillos = crear_pasillos();
    estrella = crear_estrella_central();
    bordes = crear_bordes();

//fichas
    // AMARILLO = 100..103, AZUL = 104..107, ROJO = 108..111, VERDE = 112..115
    for (int i = 0; i < 4; i++) {
        fichas[i].casilla_actual = 100 + i;  // casas amarillas
        fichas[i].color = AMARILLO;
        fichas[i].en_meta = false;
    } 
    for (int i = 4; i < 8; i++) {
        fichas[i].casilla_actual = 104 + i-4;  // casas azules
        fichas[i].color = AZUL;
        fichas[i].en_meta = false;
    } 
    for (int i = 8; i < 12; i++) {
        fichas[i].casilla_actual = 108 + i-8;  // casas rojas
        fichas[i].color = ROJO;
        fichas[i].en_meta = false;
    }
    for (int i = 12; i < 16; i++) {
        fichas[i].casilla_actual = 112 + i-12;  // casas rojas
        fichas[i].color = VERDE;
        fichas[i].en_meta = false;
    }



    GLuint vs = compilar_shader(vertex_prog, GL_VERTEX_SHADER);
    GLuint fs = compilar_shader(fragment_prog, GL_FRAGMENT_SHADER);

    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    check_errores_programa(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glUseProgram(prog);

	glfwSetScrollCallback(window, scroll_callback);

	glfwSetCursorPosCallback(window, mouse_orbital);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //INTERFAZ
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// ======================= MATRICES =========================

float fov = 45.0f;

// ======================= RENDER =========================

void render_scene() {
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ── 1. Abrir frame ImGui ──
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ── 2. TODA la UI junta aquí ──
    ImVec4 colores[4] = {
        ImVec4(0.9f, 0.2f, 0.2f, 1.0f),  // ROJO
        ImVec4(0.2f, 0.2f, 0.9f, 1.0f),  // AZUL
        ImVec4(0.2f, 0.8f, 0.2f, 1.0f),  // VERDE
        ImVec4(0.9f, 0.9f, 0.2f, 1.0f),  // AMARILLO
    };
    const char* nombres[4] = { "Rojo", "Azul", "Verde", "Amarillo" };
    ColorJugador color_actual = orden_turnos[turno_actual];

    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(200, 80));
    ImGui::Begin("##turno", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    ImGui::TextColored(colores[color_actual], "Turno: %s", nombres[color_actual]);
    if (dado_lanzado)
        ImGui::Text("Dado: %d", dado);
    else
        ImGui::Text("Pulsa ESPACIO para tirar");
    ImGui::End();




    mat4 P = perspective(glm::radians(fov), (float)ANCHO / (float)ALTO, 0.5f, 100.0f);
	// Cámara orbital
	pos_obs.x = radio * cos(angulo_h) * cos(angulo_v);
	pos_obs.y = radio * sin(angulo_h) * cos(angulo_v);
	pos_obs.z = radio * sin(angulo_v);

	mat4 V = lookAt(pos_obs, vec3(0.0f, 0.0f, 0.0f), vec3(0,0,1));	
    mat4 M = mat4(1.0f);

    glUseProgram(prog); 
    transfer_mat4("MVP", P * V * M);

	glBindVertexArray(tablero_superior.VAO);
	glDrawArrays(GL_TRIANGLES, 0, tablero_superior.Nv);

	glBindVertexArray(tablero_inferior.VAO);
	glDrawArrays(GL_TRIANGLES, 0, tablero_inferior.Nv);

	glBindVertexArray(tablero_laterales.VAO);
	glDrawArrays(GL_TRIANGLES, 0, tablero_laterales.Nv);

	glBindVertexArray(zonas_color.VAO);
	glDrawArrays(GL_TRIANGLES, 0, zonas_color.Nv);

	glBindVertexArray(pasillos.VAO);
	glDrawArrays(GL_TRIANGLES, 0, pasillos.Nv);

    glBindVertexArray(estrella.VAO);
    glDrawArrays(GL_TRIANGLES, 0, estrella.Nv);

    glBindVertexArray(bordes.VAO);
    glDrawArrays(GL_TRIANGLES, 0, bordes.Nv);

    // logica
    glBindVertexArray(casas.VAO);
    glDrawArrays(GL_TRIANGLES, 0, casas.Nv);

    //importados
    mat4 MHangar = mat4(1.0f);
    MHangar = translate(MHangar, vec3(0.0f, 0.0f, -2.0f));  // ajusta posición
    MHangar = rotate(MHangar, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    MHangar = scale(MHangar, vec3(2.0f));  
    Hangar->draw(P, V, MHangar);

    // movimiento automático cada segundo
    /*double tiempo_actual = glfwGetTime();
    if (tiempo_actual - tiempo_anterior > 0.1) {
        mover_ficha(fichas[0],1);  // mueve solo la ficha 0
        tiempo_anterior = tiempo_actual;
    }*/

    // dibujar fichas en su casilla actual
    for (int i = 0; i < 4; i++) { //AMARILLAS
        int idx = fichas[i].casilla_actual;
        vec3 pos = vec3(grafo[idx].x, grafo[idx].y, 0.05f);
        mat4 M = mat4(1.0f);
        M = translate(M, pos);
        M = translate(M, vec3(0.006f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.025f));
        R2D2->draw(P, V, M);
    }
    for (int i = 4; i < 8; i++) { //AZULES
        int idx = fichas[i].casilla_actual;
        vec3 pos = vec3(grafo[idx].x, grafo[idx].y, 0.05f);
        mat4 M = mat4(1.0f);
        M = translate(M, pos);
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.09f));
        TieInterceptor->draw(P, V, M);
    }
    for (int i = 8; i < 12; i++) { //ROJAS
        int idx = fichas[i].casilla_actual;
        vec3 pos = vec3(grafo[idx].x, grafo[idx].y, 0.05f);
        mat4 M = mat4(1.0f);
        M = translate(M, pos);
        M = translate(M, vec3(0.0f, 0.04f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = scale(M, vec3(0.03f));
        nave->draw(P, V, M);
    }
    for (int i = 12; i < 16; i++) { //VERDES
        int idx = fichas[i].casilla_actual;
        vec3 pos = vec3(grafo[idx].x, grafo[idx].y, 0.01f);
        mat4 M = mat4(1.0f);
        M = translate(M, pos);
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.012f));
        Mask->draw(P, V, M);
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

// ======================= MAIN =========================

int main(int argc, char* argv[]) {
    init_GLFW();
    window = Init_Window(prac);
    load_Opengl();
    init_scene();
    asigna_funciones_callback(window);

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        render_scene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();    
    glfwTerminate();
    return 0;
}
// ======================= CALLBACKS =========================

void ResizeCallback(GLFWwindow* window, int width, int height)
{
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    ALTO = height;
    ANCHO = width;
}

static void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, true);

	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		radio -= 0.2f;
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		radio += 0.2f;
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		radio -= 0.2f;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		radio += 0.2f;
    
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        //dado = (rand() % 6) + 1;
        dado = 5;
        dado_lanzado = true;
        std::cout << "Dado: " << dado << std::endl;
    }
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 0;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 1;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 2;
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 3;
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && dado_lanzado) {
        mover_ficha(fichas[ficha_seleccionada], dado);
        dado_lanzado = false;
        turno_actual = (turno_actual + 1) % 4;  // pasar turno
    }

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS) {
            rightMouseDown = true;

            // Reiniciar el sistema de referencia del ratón
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            lastX = xpos;
            lastY = ypos;
        }
        else if (action == GLFW_RELEASE) {
            rightMouseDown = false;
        }
    }
}



void asigna_funciones_callback(GLFWwindow* window)
{
    glfwSetWindowSizeCallback(window, ResizeCallback);
    glfwSetKeyCallback(window, KeyCallback);
}
