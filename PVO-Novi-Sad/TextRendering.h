#ifndef TEXT_RENDERING_H
#define TEXT_RENDERING_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <map>
#include <string>


struct Character {
    unsigned int TextureID;  // ID teksture za glif
    glm::ivec2 Size;         // Dimenzije glifa
    glm::ivec2 Bearing;      // Pomak od osnovne linije do leve/gornje ivice
    unsigned int Advance;    // Pomak za sledeći karakter
};


void loadFont(const char* fontPath);
void initTextRendering();
void renderText(unsigned int shader, const std::string& text, float x, float y, float scale, glm::vec3 color);

#endif // TEXT_RENDERING_H

