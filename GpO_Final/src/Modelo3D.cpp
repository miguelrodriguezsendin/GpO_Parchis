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