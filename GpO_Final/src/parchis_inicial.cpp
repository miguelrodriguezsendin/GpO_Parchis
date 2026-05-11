#include <GpO.h>
#include "Modelo3D.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>


int ANCHO = 800, ALTO = 600;
const char* prac = "Parchis Inicial";


Modelo3D* Hangar = nullptr;
Modelo3D* nave = nullptr;
Modelo3D* TieInterceptor = nullptr;
Modelo3D* R2D2 = nullptr;
Modelo3D* Mask = nullptr;
Modelo3D* CloneTurret = nullptr;
Modelo3D* building = nullptr;
Modelo3D* LightSaber = nullptr;
Modelo3D* Falcon = nullptr;

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

GLuint depthProg;
GLuint depthMapFBO;
GLuint depthMap;
const int SHADOW_W = 2048, SHADOW_H = 2048;
mat4 lightSpaceMatrix;



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
int ficha_seleccionada = -1;

int turno_actual = 0;  // 0=AMARILLO, 1=AZUL, 2=ROJO, 3=VERDE
ColorJugador orden_turnos[4] = { AMARILLO, AZUL, ROJO, VERDE };
int dado = 0;          // resultado del dado
bool dado_lanzado = false;

double tiempo_dado = 0.0;
std::string mensaje_gui = "";
double tiempo_mensaje = 0.0;
double duracion_mensaje = 2.0;

GLuint tex_zonas = 0;
GLuint tex_pasillos = 0;

// ======================= SHADERS =========================

#define GLSL(src) "#version 330 core\n" #src

const char* vertex_prog = GLSL(
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 normal;      // ← nueva entrada

out vec3 col;
out vec2 texCoord;
out vec3 fragPos;        // posición en coordenadas escena
out vec3 fragNormal;     // normal transformada
out vec4 fragPosLightSpace;

uniform mat4 MVP = mat4(1.0f);
uniform mat4 M   = mat4(1.0f);   // solo modelo (sin V ni P)
uniform mat4 lightSpaceMatrix = mat4(1.0f);

void main()
{
    gl_Position = MVP * vec4(pos, 1);
    col         = color;
    texCoord    = uv;
    fragPos     = vec3(M * vec4(pos, 1.0));
    fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);

    // transpuesta de la inversa de M para las normales (tus apuntes pág 17)
    mat4 M_adj  = transpose(inverse(M));
    fragNormal  = normalize(vec3(M_adj * vec4(normal, 0.0)));
}
);

const char* fragment_prog = GLSL(
in vec3 col;
in vec2 texCoord;
in vec3 fragPos;
in vec3 fragNormal;
in vec4 fragPosLightSpace;

out vec4 outputColor;

uniform sampler2D tex;
uniform bool usar_textura = false;

// spotlight
uniform vec3  luz_pos;  // posición
uniform vec3  luz_dir; // dirección cono
uniform float luz_cutoff; 
uniform vec3  camara_pos;
uniform float alpha_out; 
uniform sampler2D shadowMap;

// materiales
uniform float Ka;   // ambiente
uniform float Kd;   // difusa
uniform float Ks;    // especular
uniform float Kn;   // brillo especular

float calcularSombra(vec4 fragPosLS) {
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMap, proj.xy).r;
    float currentDepth = proj.z;
    float bias = 0.005;
    return (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
}
void main()
{
    vec3 base_color = usar_textura
        ? texture(tex, texCoord).rgb * col
        : col;

    // ── vectores del modelo de Phong 
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(luz_pos - fragPos);   // polígono → luz
    vec3 V = normalize(camara_pos - fragPos); // polígono → cámara

    // vector reflejado r = 2(l·n)n - l 
    vec3 R = reflect(-L, N);

    //  spotlight: ángulo entre dirección del cono y -L 
    vec3  spot_dir  = normalize(luz_dir);
    float cos_angle = dot(-L, spot_dir);  // cuánto apunta al fragmento
    float spot_factor = smoothstep(luz_cutoff - 0.05, luz_cutoff + 0.05, cos_angle);

    // componentes Phong
    float difusa   = Kd * max(dot(L, N), 0.0f);
    float especular = Ks * pow(max(dot(R, V), 0.0f), Kn);

    float sombra = calcularSombra(fragPosLightSpace);
    float iluminacion = Ka + (1.0 - sombra) * spot_factor * (difusa + especular);

    outputColor = vec4(clamp(iluminacion * base_color, 0.0f, 1.0f), alpha_out);
}
);
const char* depth_vertex_prog = GLSL(
layout(location = 0) in vec3 pos;
uniform mat4 MVP;
void main() {
    gl_Position = MVP * vec4(pos, 1.0);
}
);

const char* depth_fragment_prog = GLSL(
void main() {
    
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

GLuint cargar_textura(const char* path) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    return texID;
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

    float uvs_dummy[12] = { 0,0, 0,0, 0,0, 0,0, 0,0, 0,0 };
    GLuint uvbuf;
    glGenBuffers(1, &uvbuf);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs_dummy), uvs_dummy, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    float norm[] = {
        0,0,1,  0,0,1,  0,0,1,
        0,0,1,  0,0,1,  0,0,1
    };

    GLuint normbuf;
    glGenBuffers(1, &normbuf);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm), norm, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 6;
    return obj;
}
objeto crear_tablero_inferior()
{
    objeto obj;
    GLuint VAO;

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

    float uvs_dummy[12] = { 0,0, 0,0, 0,0, 0,0, 0,0, 0,0 };
    GLuint uvbuf;
    glGenBuffers(1, &uvbuf);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs_dummy), uvs_dummy, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    float norm[] = {
        0,0,-1,  0,0,-1,  0,0,-1,
        0,0,-1,  0,0,-1,  0,0,-1
    };
    GLuint normbuf;
    glGenBuffers(1, &normbuf);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm), norm, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

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

    float uvs_dummy[12] = { 0,0, 0,0, 0,0, 0,0, 0,0, 0,0 };
    GLuint uvbuf;
    glGenBuffers(1, &uvbuf);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs_dummy), uvs_dummy, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    float norm[] = {
        // Norte (y positivo)
        0,1,0,  0,1,0,  0,1,0,   0,1,0,  0,1,0,  0,1,0,
        // Sur (y negativo)
        0,-1,0, 0,-1,0, 0,-1,0,  0,-1,0, 0,-1,0, 0,-1,0,
        // Este (x positivo)
        1,0,0,  1,0,0,  1,0,0,   1,0,0,  1,0,0,  1,0,0,
        // Oeste (x negativo)
        -1,0,0, -1,0,0, -1,0,0,  -1,0,0, -1,0,0, -1,0,0
    };
    GLuint normbuf;
    glGenBuffers(1, &normbuf);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm), norm, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

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
    float d = 0.4f;

    float verts[72];
    float cols[72];

    int idx = 0;

    float azul[3]     = {0.2f, 0.2f, 0.9f};
    float amarillo[3] = {0.9f, 0.9f, 0.2f};
    float verde[3]    = {0.2f, 0.8f, 0.2f};
    float rojo[3]     = {0.9f, 0.2f, 0.2f};

    addRect(verts, cols, idx, -1.0f,  d, -d,  1.0f, z, azul[0], azul[1], azul[2]);
    addRect(verts, cols, idx,  d,  d,  1.0f,  1.0f, z, amarillo[0], amarillo[1], amarillo[2]);
    addRect(verts, cols, idx,  d, -1.0f,  1.0f, -d, z, verde[0], verde[1], verde[2]);
    addRect(verts, cols, idx, -1.0f, -1.0f, -d, -d, z, rojo[0], rojo[1], rojo[2]);

    //UVs: cada rectángulo tiene 6 vértices, 2 floats cada uno → 4*6*2 = 48 floats
    float uvs[48] = {
        // rect 0 azul
        0,0,  1,0,  1,1,   0,0,  1,1,  0,1,
        // rect 1 amarillo
        0,0,  1,0,  1,1,   0,0,  1,1,  0,1,
        // rect 2 verde
        0,0,  1,0,  1,1,   0,0,  1,1,  0,1,
        // rect 3 rojo
        0,0,  1,0,  1,1,   0,0,  1,1,  0,1,
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, col, uvbuf;
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

    glGenBuffers(1, &uvbuf);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    float norm_zonas[72]; // 4 rects * 6 verts * 3 floats
    for (int i = 0; i < 24; i++) {
        norm_zonas[i*3+0] = 0.0f;
        norm_zonas[i*3+1] = 0.0f;
        norm_zonas[i*3+2] = 1.0f;
    }
    GLuint normbuf;
    glGenBuffers(1, &normbuf);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm_zonas), norm_zonas, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv  = 24;
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

    float uvs_pasillos[48] = {
        0,0, 1,0, 1,1,  0,0, 1,1, 0,1,  // azul
        0,0, 1,0, 1,1,  0,0, 1,1, 0,1,  // amarillo
        0,0, 1,0, 1,1,  0,0, 1,1, 0,1,  // verde
        0,0, 1,0, 1,1,  0,0, 1,1, 0,1,  // rojo
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf, uvbuf;
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

    glGenBuffers(1, &uvbuf);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs_pasillos), uvs_pasillos, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    float norm_pas[72]; // 4 pasillos * 6 verts * 3 floats
    for (int i = 0; i < 24; i++) {
        norm_pas[i*3+0] = 0.0f;
        norm_pas[i*3+1] = 0.0f;
        norm_pas[i*3+2] = 1.0f;
    }
    GLuint normbuf2;
    glGenBuffers(1, &normbuf2);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm_pas), norm_pas, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

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

    float uvs_estrella[24] = {
        0,0, 1,0, 1,1,  // triángulo azul
        0,0, 1,0, 1,1,  // triángulo amarillo
        0,0, 1,0, 1,1,  // triángulo verde
        0,0, 1,0, 1,1,  // triángulo rojo
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint pos, colbuf, uvbuf;
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

    glGenBuffers(1, &uvbuf);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs_estrella), uvs_estrella, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    float norm_est[36]; // 4 triangulos * 3 verts * 3 floats
    for (int i = 0; i < 12; i++) {
        norm_est[i*3+0] = 0.0f;
        norm_est[i*3+1] = 0.0f;
        norm_est[i*3+2] = 1.0f;
    }
    GLuint normbuf3;
    glGenBuffers(1, &normbuf3);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf3);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm_est), norm_est, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

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

    float norm_bordes[] = {
        // Borde superior - cara horizontal (apunta arriba)
        0,0,1, 0,0,1, 0,0,1,  0,0,1, 0,0,1, 0,0,1,
        // Borde superior - cara vertical (apunta hacia Y+)
        0,1,0, 0,1,0, 0,1,0,  0,1,0, 0,1,0, 0,1,0,

        // Borde inferior - cara horizontal
        0,0,1, 0,0,1, 0,0,1,  0,0,1, 0,0,1, 0,0,1,
        // Borde inferior - cara vertical (apunta hacia Y-)
        0,-1,0, 0,-1,0, 0,-1,0,  0,-1,0, 0,-1,0, 0,-1,0,

        // Borde izquierdo - cara horizontal
        0,0,1, 0,0,1, 0,0,1,  0,0,1, 0,0,1, 0,0,1,
        // Borde izquierdo - cara vertical (apunta hacia X-)
        -1,0,0, -1,0,0, -1,0,0,  -1,0,0, -1,0,0, -1,0,0,

        // Borde derecho - cara horizontal
        0,0,1, 0,0,1, 0,0,1,  0,0,1, 0,0,1, 0,0,1,
        // Borde derecho - cara vertical (apunta hacia X+)
        1,0,0, 1,0,0, 1,0,0,  1,0,0, 1,0,0, 1,0,0,
    };

    GLuint normbuf;
    glGenBuffers(1, &normbuf);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm_bordes), norm_bordes, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

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

    std::vector<float> norms(verts.size(), 0.0f); // mismo tamaño
    for (int i = 2; i < (int)norms.size(); i += 3)
        norms[i] = 1.0f;  // z=1 para todas

    GLuint normbuf4;
    glGenBuffers(1, &normbuf4);
    glBindBuffer(GL_ARRAY_BUFFER, normbuf4);
    glBufferData(GL_ARRAY_BUFFER, norms.size()*sizeof(float), norms.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

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
    if (radio > 5.5f) radio = 5.5f; // límite máximo
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
    int primera_meta;           // índice en 68..99
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


int get_offset_ficha(int ficha_idx, float& ox, float& oy) {
    int casilla = fichas[ficha_idx].casilla_actual;
    ColorJugador color = fichas[ficha_idx].color;
    int count = 0;
    int orden = 0;
    for (int j = 0; j < 16; j++) {
        if (fichas[j].casilla_actual == casilla) {
            if (j == ficha_idx) orden = count;
            count++;
        }
    }

    ox = 0.0f; oy = 0.0f;
    if (count < 2) return count;

    // offsets según orden: 0 y 1 en -d/+d, 2 y 3 en -d/+d pero en el otro eje
    float d = 0.04f;

    // determinar si el desplazamiento principal es en Y o en X
    bool usar_oy =
        (casilla >=  0 && casilla <=  7) ||
        (casilla >= 25 && casilla <= 41) ||
        (casilla >= 59 && casilla <= 75) ||
        (casilla >= 84 && casilla <= 91);

    bool usar_ox =
        (casilla >=  8 && casilla <= 24) ||
        (casilla >= 42 && casilla <= 58) ||
        (casilla >= 76 && casilla <= 83) ||
        (casilla >= 92 && casilla <= 99);

    if (!usar_oy && !usar_ox) usar_ox = true;  // casas y esquinas → X por defecto

    if (count == 2) {
        float val = (orden == 0) ? -d : d;
        if (usar_oy) oy = val;
        else         ox = val;
    }
    else if (count == 3 || count == 4) {
        // fichas 0 y 1 → -d y +d en eje principal
        // fichas 2 y 3 → -d y +d en eje principal pero desplazados hacia un eje secundario o al otro
        if (orden == 0) { if (usar_oy) oy = -d; else ox = -d; }
        if (orden == 1) { if (usar_oy) oy =  d; else ox =  d; }
        if (orden == 2) { if (usar_oy){oy = -d; if(color == AMARILLO) ox = -d*2; else ox = d*2;} 
                          else{ox =  -d; if(color == AZUL) oy = -d*2; else oy = d*2;} }
        if (orden == 3) { if (usar_oy){oy = d; if(color == AMARILLO) ox = -d*2; else ox = d*2;} 
                          else{ox =  d; if(color == AZUL) oy = -d*2; else oy = d*2;} }
    }

    return count;
}

// cuenta fichas en una casilla
int fichas_en_casilla(int casilla) {
    int count = 0;
    for (int j = 0; j < 16; j++)
        if (fichas[j].casilla_actual == casilla) count++;
    return count;
}
bool comprobar_victoria(ColorJugador color) {
    int base_meta;
    switch (color) {
        case AMARILLO: base_meta = 68; break;
        case AZUL:     base_meta = 76; break;
        case ROJO:     base_meta = 84; break;
        case VERDE:    base_meta = 92; break;
        default: return false;
    }
    // la casilla final es la última de cada meta (base_meta + 7)
    int casilla_final = base_meta + 7;
    int count = 0;
    for (int j = 0; j < 16; j++)
        if (fichas[j].color == color && fichas[j].casilla_actual == casilla_final)
            count++;
    return count >= 4;
}
void mover_ficha(Ficha& f, int pasos) {
    for (int p = 0; p < pasos; p++) {
        int actual = f.casilla_actual;
        int siguiente = -1;

        if (actual >= 100) {
            if (pasos != 5) break;
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

        // ── bloqueo enemigo (2+ fichas enemigas) ──
        if (siguiente < 100) {
            int count_enemigo = 0;
            for (int j = 0; j < 16; j++)
                if (fichas[j].casilla_actual == siguiente && fichas[j].color != f.color)
                    count_enemigo++;
            if (count_enemigo >= 2) break;
        }
        f.casilla_actual = siguiente;
    }
}
void mostrar_mensaje(const std::string& msg) {
    mensaje_gui = msg;
    tiempo_mensaje = glfwGetTime();
}
void init_shadow_map() {
    // Framebuffer
    glGenFramebuffers(1, &depthMapFBO);

    // Textura de profundidad
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = {1,1,1,1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    // Adjuntar textura al framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
// ======================= INIT SCENE =========================

void init_scene() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);
    
    tex_zonas = cargar_textura("../data/snow_01_diff_4k.jpg");
    tex_pasillos = cargar_textura("../data/Donut.jpg");

//importados
    Hangar = new Modelo3D("../data/hangar2.obj");
    nave = new Modelo3D("../data/nave.obj"); 
    TieInterceptor = new Modelo3D("../data/TieInterceptor.obj"); 
    R2D2 = new Modelo3D("../data/R2-D2.obj");  
    Mask = new Modelo3D("../data/Mask.obj");  
    CloneTurret = new Modelo3D("../data/CloneTurret.obj");
    building = new Modelo3D("../data/building.obj");
    LightSaber = new Modelo3D("../data/LightSaberConcept.obj");
    Falcon = new Modelo3D("../data/Falcon.obj");

//logica
    construir_grafo_y_posiciones();
    casas = crear_casillas_desde_grafo(0.06f, 0.005f);


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
    Hangar->setShaderExterno(prog);
    nave->setShaderExterno(prog);
    TieInterceptor->setShaderExterno(prog);
    R2D2->setShaderExterno(prog);
    Mask->setShaderExterno(prog);
    CloneTurret->setShaderExterno(prog);
    building->setShaderExterno(prog);
    LightSaber->setShaderExterno(prog);
    Falcon->setShaderExterno(prog);

	glfwSetScrollCallback(window, scroll_callback);

	glfwSetCursorPosCallback(window, mouse_orbital);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //INTERFAZ
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Compilar shader de sombras
    GLuint dvs = compilar_shader(depth_vertex_prog, GL_VERTEX_SHADER);
    GLuint dfs = compilar_shader(depth_fragment_prog, GL_FRAGMENT_SHADER);
    depthProg = glCreateProgram();
    glAttachShader(depthProg, dvs);
    glAttachShader(depthProg, dfs);
    glLinkProgram(depthProg);
    glDeleteShader(dvs);
    glDeleteShader(dfs);

    init_shadow_map();
}

// ======================= MATRICES =========================

float fov = 45.0f;

// ======================= RENDER =========================

void render_scene() {

    // ── PASADA 1: renderizar desde la luz para generar el depth map ──
    vec3 luz_pos_v = vec3(0.0f, 0.0f, 2.0f);
    mat4 lightProj = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.5f, 10.0f);
    mat4 lightView = glm::lookAt(luz_pos_v, vec3(0,0,0), vec3(0,1,0));
    lightSpaceMatrix = lightProj * lightView;

    glViewport(0, 0, SHADOW_W, SHADOW_H);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    glUseProgram(depthProg);

    // Geometría manual — todos con M identidad
    mat4 Mident = mat4(1.0f);
    transfer_mat4("MVP", lightSpaceMatrix * Mident);
    glBindVertexArray(tablero_superior.VAO);  glDrawArrays(GL_TRIANGLES, 0, tablero_superior.Nv);
    glBindVertexArray(tablero_inferior.VAO);  glDrawArrays(GL_TRIANGLES, 0, tablero_inferior.Nv);
    glBindVertexArray(tablero_laterales.VAO); glDrawArrays(GL_TRIANGLES, 0, tablero_laterales.Nv);
    glBindVertexArray(zonas_color.VAO);       glDrawArrays(GL_TRIANGLES, 0, zonas_color.Nv);
    glBindVertexArray(pasillos.VAO);          glDrawArrays(GL_TRIANGLES, 0, pasillos.Nv);
    glBindVertexArray(estrella.VAO);          glDrawArrays(GL_TRIANGLES, 0, estrella.Nv);
    glBindVertexArray(bordes.VAO);            glDrawArrays(GL_TRIANGLES, 0, bordes.Nv);
    glBindVertexArray(casas.VAO);             glDrawArrays(GL_TRIANGLES, 0, casas.Nv);

    // Decoración
    mat4 MHangar = mat4(1.0f);
    MHangar = translate(MHangar, vec3(0.0f, 0.0f, -2.0f));
    MHangar = rotate(MHangar, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    MHangar = scale(MHangar, vec3(2.0f));
    Hangar->drawDepth(depthProg, lightSpaceMatrix, MHangar);

    mat4 MDeco1 = mat4(1.0f);
    MDeco1 = translate(MDeco1, vec3(0.7f, -0.85f, 0.0f));
    MDeco1 = rotate(MDeco1, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    MDeco1 = rotate(MDeco1, glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
    MDeco1 = scale(MDeco1, vec3(0.01f));
    CloneTurret->drawDepth(depthProg, lightSpaceMatrix, MDeco1);

    mat4 MDeco1b = mat4(1.0f);
    MDeco1b = translate(MDeco1b, vec3(0.85f, -0.7f, 0.0f));
    MDeco1b = rotate(MDeco1b, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    MDeco1b = rotate(MDeco1b, glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
    MDeco1b = rotate(MDeco1b, glm::radians(270.0f), vec3(0.0f, 1.0f, 0.0f));
    MDeco1b = scale(MDeco1b, vec3(0.01f));
    CloneTurret->drawDepth(depthProg, lightSpaceMatrix, MDeco1b);

    mat4 MDeco3 = mat4(1.0f);
    MDeco3 = translate(MDeco3, vec3(-0.85f, 0.85f, 0.0f));
    MDeco3 = rotate(MDeco3, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    MDeco3 = scale(MDeco3, vec3(0.007f));
    building->drawDepth(depthProg, lightSpaceMatrix, MDeco3);

    mat4 MDeco4 = mat4(1.0f);
    MDeco4 = translate(MDeco4, vec3(-0.85f, -0.5f, 0.03f));
    MDeco4 = rotate(MDeco4, glm::radians(76.0f), vec3(1.0f, 0.0f, 0.0f));
    MDeco4 = scale(MDeco4, vec3(0.017f));
    LightSaber->drawDepth(depthProg, lightSpaceMatrix, MDeco4);

    mat4 MDeco4b = mat4(1.0f);
    MDeco4b = translate(MDeco4b, vec3(-0.5f, -0.85f, -0.02f));
    MDeco4b = rotate(MDeco4b, glm::radians(-76.0f), vec3(0.0f, 1.0f, 0.0f));
    MDeco4b = scale(MDeco4b, vec3(0.017f));
    LightSaber->drawDepth(depthProg, lightSpaceMatrix, MDeco4b);

    mat4 MDeco5 = mat4(1.0f);
    MDeco5 = translate(MDeco5, vec3(0.85f, 0.8f, 0.0f));
    MDeco5 = rotate(MDeco5, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    MDeco5 = scale(MDeco5, vec3(0.07f));
    Falcon->drawDepth(depthProg, lightSpaceMatrix, MDeco5);

    // Fichas
    for (int i = 0; i < 4; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.15f : 0.05f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
        M = translate(M, vec3(-0.005f, -0.005f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.025f));
        R2D2->drawDepth(depthProg, lightSpaceMatrix, M);
    }
    for (int i = 4; i < 8; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.15f : 0.05f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.09f));
        TieInterceptor->drawDepth(depthProg, lightSpaceMatrix, M);
    }
    for (int i = 8; i < 12; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.15f : 0.05f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
        M = translate(M, vec3(0.0f, 0.04f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = scale(M, vec3(0.03f));
        nave->drawDepth(depthProg, lightSpaceMatrix, M);
    }
    for (int i = 12; i < 16; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.11f : 0.01f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.012f));
        Mask->drawDepth(depthProg, lightSpaceMatrix, M);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ── PASADA 2: render normal ──
    glViewport(0, 0, ANCHO, ALTO);
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImVec4 colores[4] = {
        ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
        ImVec4(0.2f, 0.2f, 0.9f, 1.0f),
        ImVec4(0.2f, 0.8f, 0.2f, 1.0f),
        ImVec4(0.9f, 0.9f, 0.2f, 1.0f),
    };
    const char* nombres[4] = { "Rojo", "Azul", "Verde", "Amarillo" };

    ImGui::SetNextWindowPos(ImVec2(ANCHO * 0.5f, 10), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::Begin("##turno", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    ColorJugador color_actual = orden_turnos[turno_actual];
    float escala = ANCHO / 800.0f;
    ImGui::SetWindowFontScale(escala * 1.5f);
    ImGui::TextColored(colores[color_actual], "Turno: %s", nombres[color_actual]);
    if (dado_lanzado) ImGui::Text("Dado: %d", dado);
    else ImGui::Text("Pulsa ESPACIO para tirar");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(10, ALTO - 120 * escala));
    ImGui::Begin("##instrucciones", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
    ImGui::SetWindowFontScale(escala);
    if (!dado_lanzado) {
        ImGui::TextColored(ImVec4(1,1,0,1), "ESPACIO"); ImGui::SameLine();
        ImGui::Text("- Tirar dado");
    } else {
        ImGui::TextColored(ImVec4(1,1,0,1), "1 2 3 4"); ImGui::SameLine();
        ImGui::Text("- Seleccionar ficha");
        ImGui::TextColored(ImVec4(1,1,0,1), "ENTER  "); ImGui::SameLine();
        ImGui::Text("- Mover ficha seleccionada %d posiciones", dado);
    }
    ImGui::End();

    double ahora = glfwGetTime();
    if (dado_lanzado && (ahora - tiempo_dado) < 2.0) {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        ImU32 color_dado;
        switch(color_actual) {
            case ROJO:     color_dado = IM_COL32(220, 50,  50,  255); break;
            case AZUL:     color_dado = IM_COL32(50,  50,  220, 255); break;
            case VERDE:    color_dado = IM_COL32(50,  200, 50,  255); break;
            case AMARILLO: color_dado = IM_COL32(220, 220, 50,  255); break;
        }
        float cx = ANCHO * 0.5f, cy = ALTO * 0.5f, s = 80.0f;
        draw->AddRectFilled(ImVec2(cx-s,cy-s), ImVec2(cx+s,cy+s), color_dado, 16.0f);
        draw->AddRect(ImVec2(cx-s,cy-s), ImVec2(cx+s,cy+s), IM_COL32(255,255,255,200), 16.0f, 0, 3.0f);
        ImU32 punto_col = IM_COL32(255,255,255,255);
        float r = 10.0f, d = s * 0.55f;
        ImVec2 pts[7] = {
            ImVec2(cx-d,cy-d), ImVec2(cx+d,cy-d), ImVec2(cx-d,cy),
            ImVec2(cx,cy),     ImVec2(cx+d,cy),    ImVec2(cx-d,cy+d), ImVec2(cx+d,cy+d)
        };
        bool layout[6][7] = {
            {0,0,0,1,0,0,0}, {1,0,0,0,0,0,1}, {1,0,0,1,0,0,1},
            {1,1,0,0,0,1,1}, {1,1,0,1,0,1,1}, {1,1,1,0,1,1,1}
        };
        for (int p = 0; p < 7; p++)
            if (layout[dado-1][p]) draw->AddCircleFilled(pts[p], r, punto_col);
    }

    double ya = glfwGetTime();
    if (!mensaje_gui.empty() && (ya - tiempo_mensaje) < duracion_mensaje) {
        ImGui::SetNextWindowPos(ImVec2(ANCHO*0.5f, ALTO*0.88f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
        ImGui::Begin("##mensaje", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
        ImGui::SetWindowFontScale((ANCHO/800.0f)*2.0f);
        ImGui::Text("%s", mensaje_gui.c_str());
        ImGui::End();
    }

    mat4 P = perspective(glm::radians(fov), (float)ANCHO/(float)ALTO, 0.5f, 100.0f);
    pos_obs.x = radio * cos(angulo_h) * cos(angulo_v);
    pos_obs.y = radio * sin(angulo_h) * cos(angulo_v);
    pos_obs.z = radio * sin(angulo_v);
    mat4 V = lookAt(pos_obs, vec3(0.0f,0.0f,0.0f), vec3(0,0,1));
    mat4 M = mat4(1.0f);

    glUseProgram(prog);
    // Conectar el shadow map en TEXTURE1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(glGetUniformLocation(prog, "shadowMap"), 1);
    glUniformMatrix4fv(glGetUniformLocation(prog, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glUniform3f(glGetUniformLocation(prog, "camara_pos"), pos_obs.x, pos_obs.y, pos_obs.z);
    glUniform3f(glGetUniformLocation(prog, "luz_pos"),  0.0f, 0.0f, 2.0f);
    glUniform3f(glGetUniformLocation(prog, "luz_dir"),  0.0f, 0.0f, -1.0f);
    glUniform1f(glGetUniformLocation(prog, "luz_cutoff"), 0.85f);
    glUniform1f(glGetUniformLocation(prog, "Ka"),  0.1f);
    glUniform1f(glGetUniformLocation(prog, "Kd"),  0.7f);
    glUniform1f(glGetUniformLocation(prog, "Ks"),  0.4f);
    glUniform1f(glGetUniformLocation(prog, "Kn"),  16.0f);
    glUniform1f(glGetUniformLocation(prog, "alpha_out"), 1.0f);
    transfer_mat4("MVP", P * V * M);
    transfer_mat4("M", M);

    glBindVertexArray(tablero_superior.VAO);  glDrawArrays(GL_TRIANGLES, 0, tablero_superior.Nv);
    glBindVertexArray(tablero_inferior.VAO);  glDrawArrays(GL_TRIANGLES, 0, tablero_inferior.Nv);
    glBindVertexArray(tablero_laterales.VAO); glDrawArrays(GL_TRIANGLES, 0, tablero_laterales.Nv);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_zonas);
    glUniform1i(glGetUniformLocation(prog, "tex"), 0);
    glUniform1i(glGetUniformLocation(prog, "usar_textura"), 1);
    glBindVertexArray(zonas_color.VAO); glDrawArrays(GL_TRIANGLES, 0, zonas_color.Nv);
    glUniform1i(glGetUniformLocation(prog, "usar_textura"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_pasillos);
    glUniform1i(glGetUniformLocation(prog, "tex"), 0);
    glUniform1i(glGetUniformLocation(prog, "usar_textura"), 1);
    glBindVertexArray(pasillos.VAO); glDrawArrays(GL_TRIANGLES, 0, pasillos.Nv);
    glBindVertexArray(estrella.VAO); glDrawArrays(GL_TRIANGLES, 0, estrella.Nv);
    glUniform1i(glGetUniformLocation(prog, "usar_textura"), 0);

    glBindVertexArray(bordes.VAO); glDrawArrays(GL_TRIANGLES, 0, bordes.Nv);
    glBindVertexArray(casas.VAO);  glDrawArrays(GL_TRIANGLES, 0, casas.Nv);

    Hangar->draw(P, V, MHangar);
    CloneTurret->draw(P, V, MDeco1);
    CloneTurret->draw(P, V, MDeco1b);
    building->draw(P, V, MDeco3);
    LightSaber->draw(P, V, MDeco4);
    LightSaber->draw(P, V, MDeco4b);
    Falcon->draw(P, V, MDeco5);

    for (int i = 0; i < 4; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.15f : 0.05f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
        M = translate(M, vec3(-0.005f, -0.005f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.025f));
        R2D2->draw(P, V, M);
    }
    for (int i = 4; i < 8; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.15f : 0.05f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(0.0f, 1.0f, 0.0f));
        M = scale(M, vec3(0.09f));
        TieInterceptor->draw(P, V, M);
    }
    for (int i = 8; i < 12; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.15f : 0.05f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
        M = translate(M, vec3(0.0f, 0.04f, 0.0f));
        M = rotate(M, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        M = scale(M, vec3(0.03f));
        nave->draw(P, V, M);
    }
    for (int i = 12; i < 16; i++) {
        int idx = fichas[i].casilla_actual;
        float ox, oy;
        get_offset_ficha(i, ox, oy);
        float z_ficha = (ficha_seleccionada == i) ? 0.11f : 0.01f;
        mat4 M = mat4(1.0f);
        M = translate(M, vec3(grafo[idx].x+ox, grafo[idx].y+oy, z_ficha));
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

    if (key == GLFW_KEY_UP && action == GLFW_PRESS)   radio -= 0.2f;
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) radio += 0.2f;
    if (key == GLFW_KEY_W && action == GLFW_PRESS)    radio -= 0.2f;
    if (key == GLFW_KEY_S && action == GLFW_PRESS)    radio += 0.2f;

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (dado_lanzado) return;
        dado = (rand() % 6) + 1;
        //dado = 5;
        tiempo_dado = glfwGetTime();
        dado_lanzado = true;

        //si todas las fichas están en casa y no ha salido 5, pasar turno
        ColorJugador color_turno = orden_turnos[turno_actual];
        bool alguna_fuera = false;
        for (int j = 0; j < 16; j++) {
            if (fichas[j].color == color_turno && fichas[j].casilla_actual < 100) {
                int ca = fichas[j].casilla_actual;
                bool en_final = (ca == 75 || ca == 83 || ca == 91 || ca == 99);
                if (!en_final) {
                    alguna_fuera = true;
                    break;
                }
            }
        }
        if (!alguna_fuera && dado != 5) {
            mostrar_mensaje("Sin fichas fuera y dado != 5, turno pasado");
            dado_lanzado = false;
            turno_actual = (turno_actual + 1) % 4;
            ficha_seleccionada = -1;
        }
    }

    if (dado_lanzado) {
        if (key == GLFW_KEY_1 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 0;
        if (key == GLFW_KEY_2 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 1;
        if (key == GLFW_KEY_3 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 2;
        if (key == GLFW_KEY_4 && action == GLFW_PRESS) ficha_seleccionada = turno_actual * 4 + 3;
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && dado_lanzado && ficha_seleccionada != -1) {

        // simular el movimiento paso a paso
        int casilla_sim = fichas[ficha_seleccionada].casilla_actual;
        bool meta_temp  = fichas[ficha_seleccionada].en_meta;
        bool movimiento_valido = true;

        for (int p = 0; p < dado; p++) {
            int sig = -1;

            if (casilla_sim >= 100) {
                if (dado != 5) { movimiento_valido = false; break; }
                if (fichas[ficha_seleccionada].color == AMARILLO) sig = 0;
                else if (fichas[ficha_seleccionada].color == AZUL)  sig = 17;
                else if (fichas[ficha_seleccionada].color == ROJO)  sig = 34;
                else if (fichas[ficha_seleccionada].color == VERDE) sig = 51;
                // salida siempre se puede con 5, no hay bloqueo en salida
                casilla_sim = sig;
                continue;
            } else if (!meta_temp && grafo[casilla_sim].next_meta != -1
                    && fichas[ficha_seleccionada].color == grafo[casilla_sim].color) {
                sig = grafo[casilla_sim].next_meta;
                meta_temp = true;
            } else {
                sig = grafo[casilla_sim].next;
            }

            if (sig == -1) break;

            // comprobar bloqueo enemigo en cada casilla intermedia Y final
            int enemigos_en_sig = 0;
            for (int j = 0; j < 16; j++)
                if (fichas[j].casilla_actual == sig && fichas[j].color != fichas[ficha_seleccionada].color)
                    enemigos_en_sig++;

            if (enemigos_en_sig >= 2) {
                movimiento_valido = false;  // bloqueada por enemigo, no puede pasar
                break;
            }

            casilla_sim = sig;
        }

        // comprobar que el destino final no esta lleno de fichas propias
        int total_en_destino = fichas_en_casilla(casilla_sim);
        bool es_meta_o_final = (casilla_sim == 75 || casilla_sim == 83 || casilla_sim == 91 || casilla_sim == 99);
        if (total_en_destino >= 2 && !es_meta_o_final) movimiento_valido = false;
        if (total_en_destino >= 4) movimiento_valido = false; 

        if (!movimiento_valido) {
            mostrar_mensaje("Movimiento invalido");
            return;  // no pierde turno
        }

        // movimiento valido, hacerlo
        mover_ficha(fichas[ficha_seleccionada], dado);
        ColorJugador color_movido = fichas[ficha_seleccionada].color;
        if (comprobar_victoria(color_movido)) {
            duracion_mensaje = 999.0;  // que se quede hasta reiniciar
            const char* nombres_color[4] = { "Rojo", "Azul", "Verde", "Amarillo" };
            mostrar_mensaje("¡¡Ha ganado el jugador " + std::string(nombres_color[color_movido]) + "!!");
            dado_lanzado = false;
            ficha_seleccionada = -1;
            return;  // bloquear el juego
        }

        // ── COMER FICHAS ENEMIGAS ──────────────────────────────
        int casilla_destino = fichas[ficha_seleccionada].casilla_actual;
        bool casilla_es_segura = (grafo[casilla_destino].tipo == SEGURO);

        if (!casilla_es_segura && casilla_destino < 68) {  // no come en meta
            for (int j = 0; j < 16; j++) {
                if (j == ficha_seleccionada) continue;
                if (fichas[j].color == fichas[ficha_seleccionada].color) continue; // misma equipo
                if (fichas[j].casilla_actual != casilla_destino) continue; // no está ahí

                // mandar a casa
                int base_casa;
                switch (fichas[j].color) {
                    case AMARILLO: base_casa = 100; break;
                    case AZUL:     base_casa = 104; break;
                    case ROJO:     base_casa = 108; break;
                    case VERDE:    base_casa = 112; break;
                    default:       base_casa = 100; break;
                }
                // buscar hueco libre en su casa
                for (int k = 0; k < 4; k++) {
                    bool ocupada = false;
                    for (int m = 0; m < 16; m++)
                        if (fichas[m].casilla_actual == base_casa + k) { ocupada = true; break; }
                    if (!ocupada) {
                        fichas[j].casilla_actual = base_casa + k;
                        fichas[j].en_meta = false;
                        std::string colorLocal;
                        switch (fichas[j].color) {
                            case AMARILLO: colorLocal = " amarilla"; break;
                            case AZUL:     colorLocal = " azul";     break;
                            case ROJO:     colorLocal = " roja";     break;
                            case VERDE:    colorLocal = " verde";    break;
                        }
                        int num_local = (j % 4) + 1; //esto es para que me diga la ficha del 1 al 4 y no del 0 al 15
                        mostrar_mensaje("Ficha " + std::to_string(num_local) + colorLocal + " comida y enviada a casa!");
                        break;
                    }
                }
            }
        }
        dado_lanzado = false;
        turno_actual = (turno_actual + 1) % 4;
        ficha_seleccionada = -1;
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
