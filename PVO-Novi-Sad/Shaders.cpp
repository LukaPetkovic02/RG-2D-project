#include "Shaders.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Kompajlira dati šejder tipa (VERTEX, FRAGMENT)
unsigned int compileShader(GLenum type, const char* source) {
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open()) {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspesno procitan fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        std::cerr << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
        return 0;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str();

    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
            << " sejder ima gresku: " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

unsigned int createShader(const char* vsSource, const char* fsSource) {
    unsigned int program = glCreateProgram();
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Objedinjeni sejder ima gresku: " << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}