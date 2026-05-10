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
    bool usar_shader_externo = false; 

    void cargarModelo(const char* path);
    void crearShaders();

public:
    Modelo3D(const char* path);
    void draw(mat4 P, mat4 V, mat4 M);
    void setShaderExterno(GLuint prog);
    GLuint getVAO() const { return VAO; }
};