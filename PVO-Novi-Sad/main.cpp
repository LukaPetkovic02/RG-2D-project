// Autor: Luka Petkovic SV 16/2021
// Opis: Protiv-vazdušna odbrana 

#define _CRT_SECURE_NO_WARNINGS
#define CRES 30
#define DRONES_LEFT 10
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ctime>
#include <random>
#include <chrono>
#include <thread>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H


unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
void setCircle(float  circle[64], float r, float xPomeraj, float yPomeraj);
static unsigned loadImageToTexture(const char* filePath);
void generateHelicopterPositions(int number);
void moveHelicoptersTowardsCityCenter(float cityCenterX, float cityCenterY, float speed);
bool checkCollision(float object1X, float object1Y, float object1Radius, float object2X, float object2Y, float object2Radius);
bool isDronOutsideScreen(float droneX, float droneY);
void mouseCallback(GLFWwindow* window, int button, int action, int mods);


struct Location {
    float x;
    float y;
};

struct Rocket {
    float x, y;
    float dirX, dirY;
    bool isFlying;
    int targetHelicopter;
};


float droneSpeed = 0.0002f;
Rocket rockets[10];

bool isSpacePressed = false;
bool wasSpacePressed = false;
bool isMapHidden = false;
Location helicopterPositions[5];
int helicoptersLeft = 5;
auto startTime = std::chrono::high_resolution_clock::now();
bool initWait = false;

std::string helicopterStrings[5] = { "A","S","D","F","G" };

float baseCenterX = 0.0f, baseCenterY = 0.0f;
bool baseCenterSet = false;
float baseCenterRadius = 0.07;

bool cityCenterSet = false;
float cityCenterX = 0.0f, cityCenterY = 0.0f;
float cityCenterRadius = 0.017;

float helicopterRadius = 0.03, droneRadius = 0.03;
int cityHits = 0;

int selectedHel = -1;
bool gameOver = false;
int countTo3 = 0;
void selectHelicopter(int helIndex) {
    if (helicopterPositions[helIndex].x < 100.0f) { // Proverava da li je helikopter na sceni
        selectedHel = helIndex;
    }
}

void normalizeVector(float& x, float& y) {
    float length = sqrt(x * x + y * y);
    if (length != 0) {
        x /= length;
        y /= length;
    }
}

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
void LoadFont(const char* fontPath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Deaktiviraj automatsko poravnanje

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }
        
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}
unsigned int VAOText, VBOText;
void RenderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAOText);

    for (char c : text) {
        if (c == ' ') {
            // Ako je karakter razmak, samo pomeri x poziciju
            x += (Characters['A'].Advance >> 6) * scale;
            continue;
        }

        Character ch = Characters[c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        glBindBuffer(GL_ARRAY_BUFFER, VBOText);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void InitTextRendering() {
    glGenVertexArrays(1, &VAOText);
    glGenBuffers(1, &VBOText);
    glBindVertexArray(VAOText);

    glBindBuffer(GL_ARRAY_BUFFER, VBOText);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int main(void)
{
    if (!glfwInit())
    {
        std::cout << "Greska pri ucitavanju GLFW biblioteke!\n";
        return 1;
    }

    generateHelicopterPositions(5);

    for (int i = 0; i < 10; ++i) {
        rockets[i] = { baseCenterX, baseCenterY, 0.0f, 0.0f, false, -1 };
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 1000;
    unsigned int wHeight = 900;
    const char wTitle[] = "Protiv-vazdusna odbrana Novog Sada";
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    int xPos = (mode->width - wWidth) / 2;
    int yPos = (mode->height - wHeight) / 2;
    window = glfwCreateWindow(wWidth, wHeight, wTitle, NULL, NULL);
    glfwSetWindowPos(window, xPos, yPos);

    if (window == NULL)
    {
        std::cout << "Greska pri formiranju prozora!\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);

    // OpenGL state
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Greska pri ucitavanju GLEW biblioteke!\n";
        return 3;
    }
   

    glfwSetMouseButtonCallback(window, mouseCallback);

    unsigned int unifiedShader = createShader("texture.vert", "texture.frag");
    unsigned int baseShader = createShader("base.vert", "base.frag");
    unsigned int cityShader = createShader("city.vert", "city.frag");
    unsigned int dronShader = createShader("dron.vert", "dron.frag");
    unsigned int textShader = createShader("text.vert", "text.frag");
    unsigned int greenShader = createShader("green.vert", "green.frag");
    int colorLoc = glGetUniformLocation(unifiedShader, "color");
    
    LoadFont("fonts/ariali.ttf");
    InitTextRendering();

    float vertices[] = {
    -1.0, -1.0,  0.0, 0.0,
     1.0, -1.0,  1.0, 0.0,
    -1.0,  1.0,  0.0, 1.0,

     1.0, -1.0,  1.0, 0.0,
     1.0,  1.0,  1.0, 1.0
    };

    unsigned int stride = (2 + 2) * sizeof(float);

    unsigned int VAO[3];
    glGenVertexArrays(3, VAO);
    unsigned int VBO[3];
    glGenBuffers(3, VBO);

    // VAO i VBO teksture -------------------------------------------------------------    
    glGenVertexArrays(1, &VAO[0]);
    glBindVertexArray(VAO[0]);
    glGenBuffers(1, &VBO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Renderovanje teksture -----------------------------------------------------------
    unsigned mapTexture = loadImageToTexture("res/novi_sad.png");
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned uTexLoc = glGetUniformLocation(unifiedShader, "uTex");
    glUniform1i(uTexLoc, 0);
    // Opis baze -----------------------------------------------------------------------

    // Opis centra Novog Sada ----------------------------------------------------------
    float cityCenterCircle[CRES * 2 + 4];


    // Opis drona ----------------------------------------------------------------------
    float blueCircle[CRES * 2 + 4];
    setCircle(blueCircle, 0.03, 0.0, 0.0);

    // VAO i VBO drona
    unsigned int VAOBlue, VBOBlue;
    glGenVertexArrays(1, &VAOBlue);
    glGenBuffers(1, &VBOBlue);
    glBindVertexArray(VAOBlue);
    glBindBuffer(GL_ARRAY_BUFFER, VBOBlue);
    glBufferData(GL_ARRAY_BUFFER, sizeof(blueCircle), blueCircle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int dronesLeft = DRONES_LEFT;

    // VAO i VBO preostalih dronova -----------------------------------------------------
    unsigned int VAOdronLeft[DRONES_LEFT];
    unsigned int VBOdronLeft[DRONES_LEFT];
    float dronLeftCircle[CRES * 2 + 4];
    for (int i = 0; i < dronesLeft; ++i) {

        setCircle(dronLeftCircle, 0.02, 0.6 + 0.04 * i, -0.8);

        glGenVertexArrays(1, &VAOdronLeft[i]);
        glGenBuffers(1, &VBOdronLeft[i]);
        glBindVertexArray(VAOdronLeft[i]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOdronLeft[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(dronLeftCircle), dronLeftCircle, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // VAO i VBO preostalih helikoptera -----------------------------------------------------
    float helicoptersLeftCircle[CRES * 2 + 4];
    unsigned int VAOhelicoptersLeft[5];
    unsigned int VBOhelicoptersLeft[5];
    for (int i = 0; i < 5; ++i) {

        setCircle(helicoptersLeftCircle, 0.02, 0.6 + 0.04 * i, -0.7);

        glGenVertexArrays(1, &VAOhelicoptersLeft[i]);
        glGenBuffers(1, &VBOhelicoptersLeft[i]);
        glBindVertexArray(VAOhelicoptersLeft[i]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOhelicoptersLeft[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(helicoptersLeftCircle), helicoptersLeftCircle, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // VAO i VBO indikatora statusa grada
    float cityStatusCircle[CRES * 2 + 4];
    unsigned int VAOcityStatus;
    unsigned int VBOcityStatus;
    setCircle(cityStatusCircle, 0.02, 0.6, -0.6);

    glGenVertexArrays(1, &VAOcityStatus);
    glGenBuffers(1, &VBOcityStatus);
    glBindVertexArray(VAOcityStatus);
    glBindBuffer(GL_ARRAY_BUFFER, VBOcityStatus);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cityStatusCircle), cityStatusCircle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    int applyTranslationLoc = glGetUniformLocation(baseShader, "applyTranslation");

    // Opis baze -----------------------------------------------------------------------
    float baseCircle[CRES * 2 + 4];
    setCircle(baseCircle, baseCenterRadius, 0.0, 0.0);

    // VAO i VBO baze
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(baseCircle), baseCircle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    int translationLocB = glGetUniformLocation(baseShader, "translation");

    // VAO i VBO grada
    setCircle(cityCenterCircle, cityCenterRadius, 0.0, 0.0);

    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cityCenterCircle), cityCenterCircle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    int translationLocC = glGetUniformLocation(cityShader, "uTranslation");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(wWidth),
        0.0f, static_cast<float>(wHeight));
    glUseProgram(textShader);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


    float greenVertices[] = {
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f
    };
    unsigned int greenIndices[] = {
    0, 1, 2,
    2, 3, 0
    };

    // VAO, VBO, EBO
    unsigned int VAOgreen, VBOgreen, EBOgreen;
    glGenVertexArrays(1, &VAOgreen);
    glGenBuffers(1, &VBOgreen);
    glGenBuffers(1, &EBOgreen);

    // Setup VAO, VBO, and EBO
    glBindVertexArray(VAOgreen);

    glBindBuffer(GL_ARRAY_BUFFER, VBOgreen);
    glBufferData(GL_ARRAY_BUFFER, sizeof(greenVertices), greenVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOgreen);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(greenIndices), greenIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);



    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            isMapHidden = true;
        }

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            isMapHidden = false;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            selectHelicopter(0);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            selectHelicopter(1);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            selectHelicopter(2);
        }

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        {
            selectHelicopter(3);
        }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        {
            selectHelicopter(4);
        }

        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(unifiedShader);
        glBindVertexArray(VAO[0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mapTexture);

        if (!isMapHidden)
        {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 5);
        }
        glBindTexture(GL_TEXTURE_2D, 0);


        glUseProgram(greenShader);
        int colorLoc = glGetUniformLocation(greenShader, "overlayColor");
        glUniform4f(colorLoc, 0.0f,1.0f,0.0f,0.25f);

        glBindVertexArray(VAOgreen);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);


        glUseProgram(baseShader);


        // Crtanje preostalih dronova
        for (int i = 0; i < dronesLeft; ++i) {
            glUniform1i(applyTranslationLoc, false);
            glBindVertexArray(VAOdronLeft[i]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 0.0, 0.0, 1.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(dronLeftCircle) / (2 * sizeof(float)));
        }

        // Crtanje indikatora preostalih helikoptera (crveno)
        for (int i = 0; i < helicoptersLeft; ++i) {
            glUniform1i(applyTranslationLoc, false);
            glBindVertexArray(VAOhelicoptersLeft[i]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 1.0, 0.0, 0.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(helicoptersLeftCircle) / (2 * sizeof(float)));
        }
        // Crtanje indikatora pogodjenih helikoptera (zeleno)
        for (int i = helicoptersLeft; i < 5; ++i) {
            glUniform1i(applyTranslationLoc, false);
            glBindVertexArray(VAOhelicoptersLeft[i]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 0.0, 1.0, 0.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(helicoptersLeftCircle) / (2 * sizeof(float)));
        }

        // Crtanje statusa grada
        glUniform1i(applyTranslationLoc, false);
        glBindVertexArray(VAOcityStatus);
        colorLoc = glGetUniformLocation(baseShader, "color");
        if (cityHits == 0) { // ako nije pogodjen - zeleno
            glUniform3f(colorLoc, 0.0, 1.0, 0.0);
        }
        else if (cityHits == 1) { // jednom pogodjen - zuto
            glUniform3f(colorLoc, 1.0, 1.0, 0.0);
        }
        else { // vise puta pogodjen - crveno
            glUniform3f(colorLoc, 1.0, 0.0, 0.0);
        }
        glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(cityStatusCircle) / (2 * sizeof(float)));

        // Renderovanje centra baze
        if (baseCenterSet) {
            glUseProgram(baseShader);
            glUniform1i(applyTranslationLoc, true); // Uključivanje translacije
            glUniform2f(translationLocB, baseCenterX, baseCenterY);

            glBindVertexArray(VAO[1]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 0.0, 0.0, 1.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(baseCircle) / (2 * sizeof(float)));
        }

        // Renderovanje centra Novog Sada
        if (cityCenterSet) {
            glUseProgram(cityShader);
            glUniform2f(translationLocC, cityCenterX, cityCenterY);

            glBindVertexArray(VAO[2]);
            colorLoc = glGetUniformLocation(cityShader, "color");
            glUniform3f(colorLoc, 1.0, 1.0, 0.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(cityCenterCircle) / (2 * sizeof(float)));
        }

        if (gameOver && cityHits >= 2) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            RenderText(textShader, "IZGUBILI STE", 200, 450, 1.5, glm::vec3(1.0f, 0.0f, 0.0f));
            
            countTo3++;
            if (countTo3 >= 4) {// Zatvori aplikaciju
                std::exit(EXIT_FAILURE);
            }
           
        }
        else if (gameOver && helicoptersLeft <= 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            RenderText(textShader, "ODBRANA JE USPJESNA!", 50, 450, 1.5, glm::vec3(1.0f, 0.0f, 0.0f));

            countTo3++;
            if (countTo3 >= 4) {// Zatvori aplikaciju
                std::exit(EXIT_FAILURE);
            }
        }


        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isSpacePressed) {
            if (selectedHel != -1 && dronesLeft > 0 && !rockets[dronesLeft - 1].isFlying) {
                dronesLeft--;

                rockets[dronesLeft].x = baseCenterX;
                rockets[dronesLeft].y = baseCenterY;
                rockets[dronesLeft].isFlying = true;
                rockets[dronesLeft].targetHelicopter = selectedHel;



                // Postavi početni smer ka helikopteru
                float targetX = helicopterPositions[selectedHel].x;
                float targetY = helicopterPositions[selectedHel].y;
                rockets[dronesLeft].dirX = targetX - baseCenterX;
                rockets[dronesLeft].dirY = targetY - baseCenterY;
                normalizeVector(rockets[dronesLeft].dirX, rockets[dronesLeft].dirY);

            }
            isSpacePressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
            isSpacePressed = false;
        }

        // Proteklo vreme od pocetka programa
        auto currentTime = std::chrono::high_resolution_clock::now();
        float elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;


        for (int i = 0; i < 10; ++i) {
            if (rockets[i].isFlying) {
                int targetHel = rockets[i].targetHelicopter; // Indeks ciljanog helikoptera

                // Ažuriraj smer ka trenutnoj poziciji helikoptera
                float currentHelX = helicopterPositions[targetHel].x;
                float currentHelY = helicopterPositions[targetHel].y;

                rockets[i].dirX = currentHelX - rockets[i].x;
                rockets[i].dirY = currentHelY - rockets[i].y;
                normalizeVector(rockets[i].dirX, rockets[i].dirY);

                // Ažuriraj poziciju rakete
                rockets[i].x += rockets[i].dirX * droneSpeed;
                rockets[i].y += rockets[i].dirY * droneSpeed;

                float distance = std::sqrt(std::pow(rockets[i].x - currentHelX, 2) + std::pow(rockets[i].y - currentHelY, 2));
               
                RenderText(textShader, std::to_string((int)(distance*1000))+"m", rockets[i].x* wWidth / 2 + wWidth / 2, rockets[i].y* wHeight / 2 + wHeight / 2 - 30, 0.3, glm::vec3(0.0f, 0.0f, 0.0f));

                // Provera sudara
                if (checkCollision(rockets[i].x, rockets[i].y, droneRadius,
                    currentHelX, currentHelY, helicopterRadius)) {

                    std::random_device rd; // Nasumično seme
                    std::mt19937 gen(rd()); // Mersenne Twister generator
                    std::uniform_real_distribution<> dist(0.0, 1.0); // Interval [0, 1]

                    // Generišemo slučajan broj
                    double randomValue = dist(gen);

                    if (randomValue <= 0.75) {
                        std::cout << "POGODAK" << std::endl;
                        rockets[i].x = 1000.0;
                        rockets[i].y = 1000.0; // sklanjamo raketu sa scene
                        helicopterPositions[targetHel].x = 1000.0;
                        helicopterPositions[targetHel].y = 1000.0; // sklanjamo helikopter sa scene
                        rockets[i].isFlying = false; // Završava let rakete
                        rockets[i].targetHelicopter = -1; // Resetuj metu

                        for (int j = 0; j < 10; j++) { // unistavamo sve ostale rakete koje su ciljale isti taj helikopter
                            if (rockets[j].isFlying && rockets[j].targetHelicopter == targetHel) {
                                rockets[j].x = 1000.0;
                                rockets[j].y = 1000.0; // sklanjamo raketu sa scene
                                rockets[j].isFlying = false; // Završava let rakete
                                rockets[j].targetHelicopter = -1; // Resetuj metu                                
                            }
                        }

                        helicoptersLeft--;
                    }
                    else { // samo raketu sklanjamo
                        std::cout << "NEMAS SRECE" << std::endl;
                        rockets[i].x = 1000.0;
                        rockets[i].y = 1000.0; // sklanjamo raketu sa scene
                        rockets[i].isFlying = false; // Završava let rakete
                        rockets[i].targetHelicopter = -1; // Resetuj metu
                    }
                }
                // Prikaz rakete
                glUseProgram(dronShader);
                glBindVertexArray(VAOBlue);
                GLint translationLoc = glGetUniformLocation(dronShader, "uTranslation");
                glUniform2f(translationLoc, rockets[i].x, rockets[i].y);
                colorLoc = glGetUniformLocation(dronShader, "color");
                glUniform3f(colorLoc, 0.0, 1.0, 0.0);
                glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(blueCircle) / (2 * sizeof(float)));
            }
        }


        for (int i = 0; i < 5; i++) {
            // Izraèunamo vektor od helikoptera do centra
            float dirX = cityCenterX - helicopterPositions[i].x;
            float dirY = cityCenterY - helicopterPositions[i].y;

            // Izraèunamo razdaljinu od koptera do centra - Pitagora
            float distance = sqrt(dirX * dirX + dirY * dirY);

            dirX /= distance;
            dirY /= distance;

            // Prilagodimo brzinu pulsiranja na osnovu udaljenosti od centra - osnovna brzina je 5.0f
            float pulseSpeed = 5.0f + 10.0f * (1.0f - distance);

            // Izraèunamo faktor pulsiranja na osnovu vremena i udaljenosti
            float pulseFactor = 0.5f + 0.5f * sin(elapsedTime * pulseSpeed);
            float redIntensity = 1.0;
            float greenIntensity = 1.0 - pulseFactor;
            float blueIntensity = 1.0 - pulseFactor;

            if (i == selectedHel) { // boji se samo ako ga niko trenutno ne gadja
                redIntensity = 1.0f;
                blueIntensity = 1.0f;
                greenIntensity = 0.0f;
            }
            glUseProgram(dronShader);
            glBindVertexArray(VAOBlue);
            GLint translationLoc = glGetUniformLocation(dronShader, "uTranslation");
            glUniform2f(translationLoc, helicopterPositions[i].x, helicopterPositions[i].y);
            colorLoc = glGetUniformLocation(dronShader, "color");
            glUniform3f(colorLoc, redIntensity, greenIntensity, blueIntensity);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(blueCircle) / (2 * sizeof(float)));

            RenderText(textShader,helicopterStrings[i], helicopterPositions[i].x* wWidth / 2 + wWidth / 2 - 5, helicopterPositions[i].y* wHeight / 2 + wHeight / 2 - 5, 0.3, glm::vec3(0.0f, 0.0f, 0.0f));

            if (checkCollision(helicopterPositions[i].x, helicopterPositions[i].y, helicopterRadius, cityCenterX, cityCenterY, cityCenterRadius)) {
                helicopterPositions[i].x = 1000.0f; // Skloni helikopter sa scene
                helicopterPositions[i].y = 1000.0f;
                std::cout << "Helicopter has hit the city" << std::endl;
                cityHits++;
                helicoptersLeft--;
                for (int j = 0; j < 10; j++) {// nema vise smisla da raketa leti ka onom koji je vec pogodio grad
                    if (rockets[j].isFlying && rockets[j].targetHelicopter == i) {
                        rockets[j].isFlying = false;
                        rockets[j].x = 1000.0;
                        rockets[j].y = 1000.0;
                    }
                }
                selectedHel = -1;
                if (cityHits >= 2) {
                    gameOver = true;
                }
            }
            if (helicoptersLeft <= 0) {
                gameOver = true;
            }
        }
        if (cityCenterSet) {
            if (!initWait) {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                initWait = true;
            }
            moveHelicoptersTowardsCityCenter(cityCenterX, cityCenterY, droneSpeed / 2);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &mapTexture);
    glDeleteBuffers(3, VBO);
    glDeleteVertexArrays(3, VAO);
    glDeleteBuffers(1, &VBOBlue);
    glDeleteVertexArrays(1, &VAOBlue);
    glDeleteProgram(unifiedShader);
    glDeleteProgram(baseShader);

    for (int i = 0; i < DRONES_LEFT; i++) {
        glDeleteVertexArrays(1, &VAOdronLeft[i]);
        glDeleteBuffers(1, &VBOdronLeft[i]);
    }

    for (int i = 0; i < 5; i++) {
        glDeleteVertexArrays(1, &VAOhelicoptersLeft[i]);
        glDeleteBuffers(1, &VBOhelicoptersLeft[i]);
    }

    glfwTerminate();
    return 0;
}

bool checkCollision(float object1X, float object1Y, float object1Radius, float object2X, float object2Y, float object2Radius) {
    float distance = std::sqrt(std::pow(object2X - object1X, 2) + std::pow(object2Y - object1Y, 2));
    return distance < (object1Radius + object2Radius);
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Dobijanje koordinata miša
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Pretvaranje koordinata miša u OpenGL prostor
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Konverzija iz piksela u normalized device coordinates (-1, 1)
        if (!baseCenterSet) {
            baseCenterX = (float)(2.0 * mouseX / width - 1.0);
            baseCenterY = (float)(1.0 - 2.0 * mouseY / height);
            baseCenterSet = true; // Centar je postavljen
            std::cout << baseCenterX << " " << baseCenterY << std::endl;
        }

        // Ako je baza već postavljena, postavi cityCenter
        else if (!cityCenterSet) {
            cityCenterX = (float)(2.0 * mouseX / width - 1.0);
            cityCenterY = (float)(1.0 - 2.0 * mouseY / height);
            cityCenterSet = true;  // Postavljamo da je city center postavljen
        }
    }
}

void moveHelicoptersTowardsCityCenter(float cityCenterX, float cityCenterY, float speed) {
    for (int i = 0; i < 5; i++) {
        // Izracunamo vektor od helikoptera do centra
        float dirX = cityCenterX - helicopterPositions[i].x;
        float dirY = cityCenterY - helicopterPositions[i].y;

        float distance = sqrt(dirX * dirX + dirY * dirY);

        // Normalizujemo vektor
        dirX /= distance;
        dirY /= distance;

        // Pomeramo helikopter ka centru odredjenom brzinom
        helicopterPositions[i].x += dirX * speed;
        helicopterPositions[i].y += dirY * speed;
    }
}

void generateHelicopterPositions(int number) {
    srand(static_cast<unsigned>(time(nullptr)));

    for (int i = 0; i < number; ++i) {
        int strana = rand() % 4;
        if (strana == 0) {
            helicopterPositions[i].x = 1;
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            helicopterPositions[i].y = std::stof(randomFloat);
        }
        else if (strana == 1) {
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            helicopterPositions[i].x = std::stof(randomFloat);
            helicopterPositions[i].y = 1;
        }
        else if (strana == 2) {
            helicopterPositions[i].x = -1;
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            helicopterPositions[i].y = std::stof(randomFloat);
        }
        else {
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            helicopterPositions[i].x = std::stof(randomFloat);
            helicopterPositions[i].y = -1;
        }
    }
}

bool isDronOutsideScreen(float droneX, float droneY)
{
    return (droneX < -1.0f || droneX > 1.0f || droneY < -1.0f || droneY > 1.0f);
}


void setCircle(float  circle[64], float r, float xPomeraj, float yPomeraj)
{
    float centerX = 0.0;
    float centerY = 0.0;

    circle[0] = centerX + xPomeraj;
    circle[1] = centerY + yPomeraj;

    for (int i = 0; i <= CRES; i++) {
        circle[2 + 2 * i] = centerX + xPomeraj + r * cos((3.141592 / 180) * (i * 360 / CRES)); // Xi pomeren za xPomeraj
        circle[2 + 2 * i + 1] = centerY + yPomeraj + r * sin((3.141592 / 180) * (i * 360 / CRES)); // Yi pomeren za yPomeraj
    }
    //Crtali smo od "nultog" ugla ka jednom pravcu, sto nam ostavlja prazno mjesto od poslednjeg tjemena kruznice do prvog,
    //pa da bi ga zatvorili, koristili smo <= umjesto <, sto nam dodaje tjeme (cos(0), sin(0))
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        //std::cout << "Uspesno procitan fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);

    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}
static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}