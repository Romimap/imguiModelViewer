#ifndef __RENDERER3D__
#define __RENDERER3D__

#include "imgui.h"
#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <sstream>
#include <iostream>
#define BUFFER_OFFSET(i) ((char *)NULL + (i))


struct VertexData {
    glm::vec3 position;
    glm::vec3 color;
};

class Renderer3D {
private:
    GLuint _FBO = 0;
    GLuint _outputColor = 0;
    GLuint _outputDepth = 0;
    ImVec2 _size;

    GLuint _VBO;
    GLuint _shaderProgram;

    glm::mat4x4 _projectionMatrix;
    glm::mat4x4 _viewMatrix;
    glm::mat4x4 _modelMatrix;


public:
    Renderer3D(ImVec2 size, char* model, char* vertexShader, char* fragmentShader);
    ~Renderer3D();
    void Draw();

private:
    //void LoadMesh(char* model);
    void MakeShaderProgram(char* fragmentShader, char* vertexShader);
};


Renderer3D::Renderer3D(ImVec2 size, char* model = "./models/cube.obj", char* fragmentShader = "./shaders/fshader.glsl", char* vertexShader = "./shaders/vshader.glsl") : _size(size) {
    MakeShaderProgram(fragmentShader, vertexShader);

    //LoadMesh(model);
    std::vector<VertexData> positions;
    VertexData a, b, c;
    a.position = glm::vec3(-1, -1, 0);
    a.color = glm::vec3(1, 0, 0);
    b.position = glm::vec3(0, 1, 0);
    b.color = glm::vec3(0, 1, 0);
    c.position = glm::vec3(1, -1, 0);
    c.color = glm::vec3(0, 0, 1);
    
    positions.push_back(a);
    positions.push_back(b);
    positions.push_back(c);

    glGenBuffers(1, &_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(VertexData), positions.data(), GL_STATIC_DRAW);
    
    
    //We create a new buffer
    glGenFramebuffers(1, &_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO); //Bind

    //Create & Attach a texture to it
    glGenTextures(1, &_outputColor);
    glBindTexture(GL_TEXTURE_2D, _outputColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _size.x, _size.y, 0,  GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outputColor, 0);  
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");




    
    

    _projectionMatrix = glm::perspective<float>(glm::radians(55.0), _size.x / _size.y, 0.1f, 100.0f);
    _modelMatrix = glm::mat4x4(1.0f);
    _viewMatrix = glm::mat4x4(1.0f);


    //Back to the default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


Renderer3D::~Renderer3D() {
    glDeleteFramebuffers(1, &_FBO);
    glDeleteTextures(1, &_outputColor);
}


void Renderer3D::Draw() {
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO); //Bind
    glViewport(0, 0, _size.x, _size.y);

    glClearColor(.5, .5, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
  
    glUseProgram(_shaderProgram);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(float) * 3));
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); //Unbind

    ImGui::Image((ImTextureID)_outputColor, _size);
}


std::vector<std::string> splitstr (std::string str, std::string del) {
    size_t pos = 0;
    std::string token;
    std::vector<std::string> split;
    while ((pos = str.find(del)) != std::string::npos) {
        token = str.substr(0, pos);
        split.push_back(token);
        str.erase(0, pos + del.length());
    }
    split.push_back(str);
    return split;
}

// void Renderer3D::LoadMesh(char* model) {
//     std::ifstream file(model);
//     assert (file.is_open());

//     std::vector<glm::vec3> positions;
//     std::vector<glm::vec2> uvs;
//     std::vector<glm::vec3> normals;

//     std::map<std::string, int> vertexIds;


//     std::string line;
//     while (std::getline(file, line)) {
//         std::stringstream stream(line);
//         std::string command;
//         stream >> command;

//         float x, y, z;
//         if (command.compare("v") == 0) {
//             stream >> x >> y >> z;
//             positions.push_back(glm::vec3(x, y, z));
//         } else if (command.compare("vt") == 0) {            
//             stream >> x >> y;
//             uvs.push_back(glm::vec2(x, y));
//         } else if (command.compare("vn") == 0) {
//             stream >> x >> y >> z;
//             normals.push_back(glm::vec3(x, y, z));
//         } else if (command.compare("f") == 0) {
//             std::vector<std::string> split = splitstr(line, " ");
//             for (int i = 0; i < 3; i++) {
//                 std::vector<std::string> nmbrs = splitstr(split[i + 1], "/");
//                 int a, b, c;
//                 a = atoi(nmbrs[0].c_str()) - 1;
//                 b = atoi(nmbrs[1].c_str()) - 1;
//                 c = atoi(nmbrs[2].c_str()) - 1;

//                 std::string key = std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c);
//                 if (!vertexIds.count(key)) {
//                     VertexData vdata;

//                     vdata.position = positions[a];
//                     vdata.uv = uvs[b];
//                     vdata.normal = normals[c];

//                     vertexIds.emplace(key, vertices.size());
//                     //vertices.push_back(vdata);
//                 }

//                 indices.push_back(vertexIds[key]);
//             }
//         }
//         //TODO: THAT

//         //glGenBuffers(1, &_VAO);
//         //glBindBuffer(GL_ARRAY_BUFFER, _VAO);
//         //glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexData), vertices.data(), GL_STATIC_DRAW);
// //
//         //glGenBuffers(1, &_indexBuffer);
//         //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
//         //glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
//     }

// }


void Renderer3D::MakeShaderProgram(char* fragmentShader, char* vertexShader) {
    //SHADER
    _shaderProgram = glCreateProgram();
    GLint success;

    //--VSHADER
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    std::ifstream vCodeFile(vertexShader);
    std::stringstream vCodeStream;
    vCodeStream << vCodeFile.rdbuf();
    std::string vCodeStr = vCodeStream.str();

    const GLchar* vCode[1];
    vCode[0] = vCodeStr.c_str();

    GLint vLenght[1];
    vLenght[0] = vCodeStr.length();

    glShaderSource(vShader, 1, vCode, vLenght);
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(vShader, sizeof(InfoLog), NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_VERTEX_SHADER, InfoLog);
        exit(1);
    }
    glAttachShader(_shaderProgram, vShader);

    //--FSHADER
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::ifstream fCodeFile(fragmentShader);
    std::stringstream fCodeStream;
    fCodeStream << fCodeFile.rdbuf();
    std::string fCodeStr = fCodeStream.str();

    const GLchar* fCode[1];
    fCode[0] = fCodeStr.c_str();

    GLint fLenght[1];
    fLenght[0] = fCodeStr.length();


    glShaderSource(fShader, 1, fCode, fLenght);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(fShader, sizeof(InfoLog), NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_FRAGMENT_SHADER, InfoLog);
        exit(1);
    }
    glAttachShader(_shaderProgram, fShader);

    //--LINKING
    glLinkProgram(_shaderProgram);
    glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
    if (success == 0) {
        GLchar ErrorLog[1024];
        glGetProgramInfoLog(_shaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
    }
}


#endif