#include <GpO.h>

int ANCHO = 800, ALTO = 600;
const char* prac = "Parchis Inicial";

GLFWwindow* window;
GLuint prog;
objeto tablero;

//movimiento camara variables
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp    = vec3(0.0f, 1.0f, 0.0f);
vec3 pos_obs     = vec3(0.0f, 0.0f, 3.0f);


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

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;

    vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = normalize(front);
}

// ======================= CREAR CUADRADO =========================


objeto crear_cuadrado() {
    objeto obj;
    GLuint VAO, buffer_pos, buffer_col;

    GLfloat pos_data[][3] = {
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},

        {-1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f}
    };

    GLfloat color_data[][3] = {
        {0.8f, 0.8f, 0.8f},
        {0.8f, 0.8f, 0.8f},
        {0.8f, 0.8f, 0.8f},

        {0.8f, 0.8f, 0.8f},
        {0.8f, 0.8f, 0.8f},
        {0.8f, 0.8f, 0.8f}
    };

	

    glGenBuffers(1, &buffer_pos);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pos_data), pos_data, GL_STATIC_DRAW);

    glGenBuffers(1, &buffer_col);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_data), color_data, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);

    glBindVertexArray(0);

    obj.VAO = VAO;
    obj.Nv = 6;
    return obj;
}


objeto crear_cuadricula(int N)
{
    objeto obj;
    GLuint VAO, buffer_pos, buffer_col;

    int MAX_VERTS = 6 * N * N;

    GLfloat* pos_data = new GLfloat[MAX_VERTS * 3];
    GLfloat* col_data = new GLfloat[MAX_VERTS * 3];

    int idx = 0;
    float step = 2.0f / N; //cuanto ocupa cada cuadrado

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            float x0 = -1.0f + j * step;
            float x1 = x0 + step;
            float y0 = -1.0f + i * step;
            float y1 = y0 + step;

            // Color base (fondo)
			float r,g,b;
			r = g = b = 0.9f;

			// ================= ZONAS GRANDES DE COLOR (CASAS) =================
			// Rojo (arriba izquierda): filas 0..4, columnas 0..4
			if (i <= 4 && j <= 4) {
				r = 0.9f; g = 0.2f; b = 0.2f;
			}
			// Azul (arriba derecha): filas 0..4, columnas 10..14
			else if (i <= 4 && j >= 10) {
				r = 0.2f; g = 0.2f; b = 0.9f;
			}
			// Verde (abajo izquierda): filas 10..14, columnas 0..4
			else if (i >= 10 && j <= 4) {
				r = 0.2f; g = 0.8f; b = 0.2f;
			}
			// Amarillo (abajo derecha): filas 10..14, columnas 10..14
			else if (i >= 10 && j >= 10) {
				r = 0.9f; g = 0.9f; b = 0.2f;
			}

			// ================= CASAS INTERIORES (CUADRADOS BLANCOS) =================
			// Rojo: centro de su cuadrante (1..3, 1..3)
			if (i >= 1 && i <= 3 && j >= 1 && j <= 3) {
				r = g = b = 1.0f;
			}
			// Azul
			if (i >= 1 && i <= 3 && j >= 11 && j <= 13) {
				r = g = b = 1.0f;
			}
			// Verde
			if (i >= 11 && i <= 13 && j >= 1 && j <= 3) {
				r = g = b = 1.0f;
			}
			// Amarillo
			if (i >= 11 && i <= 13 && j >= 11 && j <= 13) {
				r = g = b = 1.0f;
			}

			// ================= CAMINO PRINCIPAL (CRUZ) =================
			// Fila central (7), excepto dentro de las casas
			if (i == 7 && j >= 0 && j <= 14 && !(j <= 4 || j >= 10)) {
				r = g = b = 0.8f;
			}
			// Columna central (7), excepto dentro de las casas
			if (j == 7 && i >= 0 && i <= 14 && !(i <= 4 || i >= 10)) {
				r = g = b = 0.8f;
			}

			// ================= FILAS DE META (HOME ROWS) =================
			// Rojo: columna 7, filas 1..5
			if (j == 7 && i >= 1 && i <= 5) {
				r = 0.9f; g = 0.2f; b = 0.2f;
			}
			// Azul: fila 7, columnas 9..13
			if (i == 7 && j >= 9 && j <= 13) {
				r = 0.2f; g = 0.2f; b = 0.9f;
			}
			// Amarillo: columna 7, filas 9..13
			if (j == 7 && i >= 9 && i <= 13) {
				r = 0.9f; g = 0.9f; b = 0.2f;
			}
			// Verde: fila 7, columnas 1..5
			if (i == 7 && j >= 1 && j <= 5) {
				r = 0.2f; g = 0.8f; b = 0.2f;
			}

			// ================= CASILLAS DE SALIDA (START) =================
			// Rojo: justo antes de su fila de meta
			// Ejemplo: fila 6, col 7
			if (i == 6 && j == 7) {
				r = 1.0f; g = 0.0f; b = 0.0f;
			}
			// Azul: col 8, fila 7
			if (i == 7 && j == 8) {
				r = 0.0f; g = 0.0f; b = 1.0f;
			}
			// Amarillo: fila 8, col 7
			if (i == 8 && j == 7) {
				r = 1.0f; g = 1.0f; b = 0.0f;
			}
			// Verde: fila 7, col 6
			if (i == 7 && j == 6) {
				r = 0.0f; g = 1.0f; b = 0.0f;
			}

			// ================= CENTRO (ESTRELLA) =================
			// Lo simplificamos como un bloque blanco 3x3
			if (i >= 6 && i <= 8 && j >= 6 && j <= 8) {
				r = g = b = 1.0f;
			}



            // 6 vértices por casilla
            float verts[6][3] = {
                {x0, y0, 0}, {x1, y0, 0}, {x1, y1, 0},
                {x0, y0, 0}, {x1, y1, 0}, {x0, y1, 0}
            };

            for (int k = 0; k < 6; k++)
            {
                pos_data[(idx+k)*3 + 0] = verts[k][0];
                pos_data[(idx+k)*3 + 1] = verts[k][1];
                pos_data[(idx+k)*3 + 2] = verts[k][2];

                col_data[(idx+k)*3 + 0] = r;
                col_data[(idx+k)*3 + 1] = g;
                col_data[(idx+k)*3 + 2] = b;
            }

            idx += 6;
        }
    }

    glGenBuffers(1, &buffer_pos);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
    glBufferData(GL_ARRAY_BUFFER, idx * 3 * sizeof(float), pos_data, GL_STATIC_DRAW);

    glGenBuffers(1, &buffer_col);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
    glBufferData(GL_ARRAY_BUFFER, idx * 3 * sizeof(float), col_data, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);

    glBindVertexArray(0);

    delete[] pos_data;
    delete[] col_data;

    obj.VAO = VAO;
    obj.Nv = idx;
    return obj;
}


// ======================= INIT SCENE =========================

void init_scene() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    //tablero = crear_cuadrado();
	tablero = crear_cuadricula(15);  // 10x10 casillas


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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

}

// ======================= MATRICES =========================

/*vec3 pos_obs = vec3(3.0f, 0.0f, 3.0f);
vec3 target  = vec3(0.0f, 0.0f, 0.0f);
vec3 up      = vec3(0, 0, 1);*/

float fov = 45.0f, aspect = 4.0f/3.0f;

// ======================= RENDER =========================

void render_scene() {
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    mat4 P = perspective(glm::radians(fov), aspect, 0.5f, 20.0f);
    mat4 V = lookAt(pos_obs, pos_obs + cameraFront, cameraUp);
    mat4 M = mat4(1.0f);

    transfer_mat4("MVP", P * V * M);

    glBindVertexArray(tablero.VAO);
    glDrawArrays(GL_TRIANGLES, 0, tablero.Nv);
    glBindVertexArray(0);
}

// ======================= MAIN =========================

int main(int argc, char* argv[]) {
    init_GLFW();
    window = Init_Window(prac);
    load_Opengl();
    init_scene();

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        render_scene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

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
}

void asigna_funciones_callback(GLFWwindow* window)
{
    glfwSetWindowSizeCallback(window, ResizeCallback);
    glfwSetKeyCallback(window, KeyCallback);
}
