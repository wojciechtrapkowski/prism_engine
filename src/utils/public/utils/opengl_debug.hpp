#pragma once


#include <GLFW/glfw3.h>

#include <iostream>

#define GLCheck(x)                                                                                                                                             \
    do {                                                                                                                                                       \
        x;                                                                                                                                                     \
        GLenum err = glGetError();                                                                                                                             \
        if (err != GL_NO_ERROR) {                                                                                                                              \
            std::cerr << "OpenGL Error " << err << " at " << #x << std::endl;                                                                                  \
        }                                                                                                                                                      \
    } while (0)