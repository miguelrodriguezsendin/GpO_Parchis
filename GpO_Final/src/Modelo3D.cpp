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
uniform vec3 lightDir;

void main() {

    vec3 N = normalize(vNormal);

    float diffuse = max(dot(N, lightDir), 0.2);

    vec3 finalColor = vColor * diffuse;

    color = vec4(finalColor, 1.0);
}
);

// ===================== CONSTRUCTOR =====================

Modelo3D::Modelo3D(const char* path) {
    cargarModelo(path);
    shaderProgram = 0;  // se asignará con setShaderExterno
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

                data.push_back(v.x); data.push_back(v.y); data.push_back(v.z);  // pos → loc 0
                data.push_back(diffuse.r); data.push_back(diffuse.g); data.push_back(diffuse.b);  // color → loc 1
                data.push_back(0.0f); data.push_back(0.0f);
                data.push_back(n.x); data.push_back(n.y); data.push_back(n.z);
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

    // pos → location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)0);

    // color → location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(3*sizeof(float)));

    // uv → location 2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(6*sizeof(float)));

    // normal → location 3
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(8*sizeof(float)));

    glBindVertexArray(0);

    numVertices = data.size() / 11;
}

// ===================== SHADERS =====================

// ===================== DRAW =====================

void Modelo3D::setShaderExterno(GLuint prog) {
    shaderProgram = prog;
    usar_shader_externo = true;
}

void Modelo3D::draw(mat4 P, mat4 V, mat4 M) {
    glUseProgram(shaderProgram);
    transfer_mat4("MVP", P * V * M);
    transfer_mat4("M", M);

    if (!usar_shader_externo) {
        // solo si usa shader propio
        glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"), 0.5f, 1.0f, 0.8f);
    }

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
    glBindVertexArray(0);
}
void Modelo3D::drawDepth(GLuint depthProg, mat4 lightSpaceMatrix, mat4 M) {
    glUseProgram(depthProg);
    transfer_mat4("MVP", lightSpaceMatrix * M);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
    glBindVertexArray(0);
}