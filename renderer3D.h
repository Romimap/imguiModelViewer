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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define BUFFER_OFFSET(i) ((char *)NULL + (i))


struct VertexData {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 tangeant;
    glm::vec3 bitangeant;
};

class Renderer3D {
private:
    GLuint _FBO = 0;
    GLuint _outputColor = 0;
    GLuint _outputDepth = 0;
    ImVec2 _size;

    GLuint _VBO;
    GLuint _shaderProgram;
    GLuint _vShader = 0;
    GLuint _fShader = 0;
    GLuint _b = 0;
    GLuint _m = 0;
    GLuint _c = 0;

    glm::mat4x4 _projectionMatrix;
    glm::mat4x4 _viewMatrix;
    glm::mat4x4 _modelMatrix;

    std::vector<VertexData> _vertices;

    glm::vec3 *_cameraPosition;

    float s = 25;
    int mip_levels = 8;
    int save_n_first_frames = 0;



public:
    Renderer3D(ImVec2 size, glm::vec3 &cameraPosition, const char* model, const char* vertexShader, const char* fragmentShader);
    ~Renderer3D();
    void Draw(ImVec2 size, ImVec4 clearColor, float dt, float t);
    std::string SetFShader(const std::string &code);
    std::string SetVShader(const std::string &code);

    void SetAlbedo(const char* path);
    void SetRoughness(const char* path);
    void SetNormal(const char* path);

    glm::mat4 getProjectionMatrix() {return _projectionMatrix;}
    glm::mat4 getViewMatrix() {return _viewMatrix;}
    glm::mat4 getModelMatrix() {return _modelMatrix;}

    void Screenshot (const char* path);

    void SetV(const char* path);
    void SetC(const char* path);

private:
    void LoadMesh(const char* model);
    void MakeShaderProgram(const char* fragmentShader, const char* vertexShader);
    void Normalize(float* data, int w, int h, int nbC);
};


Renderer3D::Renderer3D(ImVec2 size, glm::vec3 &cameraPosition, const char* model = "./models/cube.obj", const char* fragmentShader = "./shaders/fshader.glsl", const char* vertexShader = "./shaders/vshader.glsl") : _size(size), _cameraPosition(&cameraPosition) {
    MakeShaderProgram(fragmentShader, vertexShader);

    LoadMesh(model);
    
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

    glGenTextures(1, &_outputDepth);
    glBindTexture(GL_TEXTURE_2D, _outputDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _size.x, _size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _outputDepth, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");
    
    _projectionMatrix = glm::perspective<float>(glm::radians(40.0), _size.x / _size.y, 0.1f, 1000.0f);
    _modelMatrix = glm::mat4x4(1.0f);
    _viewMatrix = glm::lookAt(*_cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));


    //Back to the default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Renderer3D::~Renderer3D() {
    glDeleteFramebuffers(1, &_FBO);
    glDeleteTextures(1, &_outputColor);
}

void Renderer3D::Screenshot (const char* path) {
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO); //Bind
    glViewport(0, 0, _size.x, _size.y);

    char *pixels = new char[3 * (int)_size.x * (int)_size.y];
    glReadPixels(0, 0, (int)_size.x, (int)_size.y, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    stbi_flip_vertically_on_write(1);
    stbi_write_bmp(path, (int)_size.x, (int)_size.y, 3, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Renderer3D::SetV (const char* path) {
    printf("SETV : %s\n", path);
    {
        glGenTextures(1, &_b);
        glBindTexture(GL_TEXTURE_2D, _b);
        int w, h, nbC;
        unsigned char *data = stbi_load(path, &w, &h, &nbC, 0);
        float *fdata = (float*)calloc(w * h * nbC, sizeof(float));
        for (int i = 0; i < w * h * nbC; i++) {
            fdata[i] = data[i] / 255.0 - 0.5;
        }
        //Normalize(fdata, w, h, nbC);
        printf("%d %d %d\n", w, h, nbC);
        glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_RGB32F, w, h);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, fdata);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    {
        glGenTextures(1, &_m);
        glBindTexture(GL_TEXTURE_2D, _m);
        int w, h, nbC;
        unsigned char *data = stbi_load(path, &w, &h, &nbC, 0); 
        float *fdata = (float*)calloc(w*h*nbC, sizeof(float));
        for (int i = 0; i < w * h * nbC; i++) {
            fdata[i] = data[i] / 255.0 - 0.5;
        }
        for (int i = 0; i < w * h * nbC; i++) {
            fdata[i] = fdata[i] * fdata[i];
        }
        glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_RGB32F, w, h);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, fdata);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
}

void Renderer3D::SetC (const char* path) {
    printf("SETC : %s\n", path);
    {
        glGenTextures(1, &_c);
        glBindTexture(GL_TEXTURE_2D, _c);
        int w, h, nbC;
        unsigned char *data = stbi_load(path, &w, &h, &nbC, 0);
        float *fdata = (float*)calloc(w*h*nbC, sizeof(float));
        for (int i = 0; i < w * h * nbC; i++) {
            fdata[i] = data[i] / 255.0;
        }
        printf("%d %d %d\n", w, h, nbC);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, w, h);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, fdata);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
}

void Renderer3D::Draw(ImVec2 size, ImVec4 clearColor, float dt, float t) {
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO); //Bind
    if (_size.x != size.x || _size.y != size.y) {
        _size = size;
        //Create & Attach a texture to it
        glGenTextures(1, &_outputColor);
        glBindTexture(GL_TEXTURE_2D, _outputColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _size.x, _size.y, 0,  GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outputColor, 0);  
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers);

        glGenTextures(1, &_outputDepth);
        glBindTexture(GL_TEXTURE_2D, _outputDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _size.x, _size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _outputDepth, 0);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");
        
        _projectionMatrix = glm::perspective<float>(glm::radians(55.0), _size.x / _size.y, 0.1f, 1000.0f);        
    }
    _viewMatrix = glm::lookAt(*_cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    glViewport(0, 0, _size.x, _size.y);


    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    glUseProgram(_shaderProgram);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(float) * 3));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(float) * 5));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(float) * 8));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(float) * 11));

    glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(_modelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(_viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(_projectionMatrix));
    glUniform3f(glGetUniformLocation(_shaderProgram, "cameraPosition"), _cameraPosition->x, _cameraPosition->y, _cameraPosition->z);
    glUniform1f(glGetUniformLocation(_shaderProgram, "DTIME"), dt);
    glUniform1f(glGetUniformLocation(_shaderProgram, "TIME"), t);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _b);
    glUniform1i(glGetUniformLocation(_shaderProgram, "b"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _m);
    glUniform1i(glGetUniformLocation(_shaderProgram, "m"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _c);
    glUniform1i(glGetUniformLocation(_shaderProgram, "c"), 2);

    glDrawArrays(GL_TRIANGLES, 0, _vertices.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); //Unbind

    if (save_n_first_frames > 0) {
        std::string frameName = "screenshots/image_sequence/";
        frameName += std::to_string(save_n_first_frames);
        frameName += ".bmp";
        Screenshot(frameName.c_str());
        save_n_first_frames--;
    }

    ImGui::Image((ImTextureID)_outputColor, _size, ImVec2(0, 1), ImVec2(1, 0));
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

void Renderer3D::LoadMesh(const char* model) {
    std::ifstream file(model);
    assert (file.is_open());

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3>* tangents[4096];
    std::vector<glm::vec3>* bitangents[4096];

    for (int i = 0; i < 4096; i++) {
        tangents[i] = new std::vector<glm::vec3>();
        bitangents[i] = new std::vector<glm::vec3>();
    }

    std::map<std::string, int> vertexIds;


    std::string line;
    while (std::getline(file, line)) {
        std::stringstream stream(line);
        std::string command;
        stream >> command;

        float x, y, z;
        if (command.compare("v") == 0) {
            stream >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        } else if (command.compare("vt") == 0) {            
            stream >> x >> y;
            uvs.push_back(glm::vec2(x, y));
        } else if (command.compare("vn") == 0) {
            stream >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
        } else if (command.compare("f") == 0) {
            std::vector<std::string> split = splitstr(line, " ");
            for (int i = 0; i < 3; i++) {
                std::vector<std::string> nmbrs = splitstr(split[i + 1], "/");
                int a, b, c;
                a = atoi(nmbrs[0].c_str()) - 1;
                b = atoi(nmbrs[1].c_str()) - 1;
                c = atoi(nmbrs[2].c_str()) - 1;

                std::string key = std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c);
              
                VertexData vdata;

                vdata.position = positions[a];
                vdata.uv = uvs[b];
                vdata.normal = normals[c];

                _vertices.push_back(vdata);
            }

            VertexData *v1 = &_vertices.at(_vertices.size() - 3);
            VertexData *v2 = &_vertices.at(_vertices.size() - 2);
            VertexData *v3 = &_vertices.at(_vertices.size() - 1);

            glm::vec3 edge1 = v2->position - v1->position;
            glm::vec3 edge2 = v3->position - v1->position;
            glm::vec2 deltaUV1 = v2->uv - v1->uv;
            glm::vec2 deltaUV2 = v3->uv - v1->uv;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            glm::vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            glm::vec3 bitangent;
            bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


            v1->tangeant = tangent;
            v2->tangeant = tangent;
            v3->tangeant = tangent;
            v1->bitangeant = bitangent;
            v2->bitangeant = bitangent;
            v3->bitangeant = bitangent;
        }

        glGenBuffers(1, &_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, _VBO);
        glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(VertexData), _vertices.data(), GL_STATIC_DRAW);
    }

    //Free the memory
    for (int i = 0; i < 4096; i++) {
        delete tangents[i];
        delete bitangents[i];
    }
}

std::string Renderer3D::SetFShader(const std::string &code) {
    GLchar infoLog[1024];
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

    const GLchar* fCode[1];
    fCode[0] = code.c_str();

    GLint fLenght[1];
    fLenght[0] = code.length();

    glShaderSource(fShader, 1, fCode, fLenght);
    glCompileShader(fShader);
    GLint success;
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fShader, 1024, NULL, infoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_FRAGMENT_SHADER, infoLog);
        return std::string(infoLog);
    }

    glDetachShader(_shaderProgram, _fShader);
    glAttachShader(_shaderProgram, fShader);

    glLinkProgram(_shaderProgram);
    glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
    if (success == 0) {
        glGetProgramInfoLog(_shaderProgram, 1024, NULL, infoLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", infoLog);
        glDetachShader(_shaderProgram, fShader);
        glAttachShader(_shaderProgram, _fShader);
        return std::string(infoLog);
    }

    _fShader = fShader;

    return std::string("");
}


std::string Renderer3D::SetVShader(const std::string &code) {
    GLchar infoLog[1024];
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);

    const GLchar* vCode[1];
    vCode[0] = code.c_str();

    GLint vLenght[1];
    vLenght[0] = code.length();

    glShaderSource(vShader, 1, vCode, vLenght);
    glCompileShader(vShader);
    GLint success;
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vShader, 1024, NULL, infoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_VERTEX_SHADER, infoLog);
        return std::string(infoLog);
    }

    glDetachShader(_shaderProgram, _vShader);
    glAttachShader(_shaderProgram, vShader);

    glLinkProgram(_shaderProgram);
    glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
    if (success == 0) {
        glGetProgramInfoLog(_shaderProgram, 1024, NULL, infoLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", infoLog);
        glDetachShader(_shaderProgram, vShader);
        glAttachShader(_shaderProgram, _vShader);
        return std::string(infoLog);
    }

    _vShader = vShader;

    return std::string("");
}


void Renderer3D::MakeShaderProgram(const char* fragmentShader, const char* vertexShader) {
    //SHADER
    _shaderProgram = glCreateProgram();
    GLint success;

    //--VSHADER
    _vShader = glCreateShader(GL_VERTEX_SHADER);
    std::ifstream vCodeFile(vertexShader);
    std::stringstream vCodeStream;
    vCodeStream << vCodeFile.rdbuf();
    std::string vCodeStr = vCodeStream.str();

    const GLchar* vCode[1];
    vCode[0] = vCodeStr.c_str();

    GLint vLenght[1];
    vLenght[0] = vCodeStr.length();

    glShaderSource(_vShader, 1, vCode, vLenght);
    glCompileShader(_vShader);
    glGetShaderiv(_vShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(_vShader, sizeof(InfoLog), NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_VERTEX_SHADER, InfoLog);
        exit(1);
    }
    glAttachShader(_shaderProgram, _vShader);

    //--FSHADER
    _fShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::ifstream fCodeFile(fragmentShader);
    std::stringstream fCodeStream;
    fCodeStream << fCodeFile.rdbuf();
    std::string fCodeStr = fCodeStream.str();

    const GLchar* fCode[1];
    fCode[0] = fCodeStr.c_str();

    GLint fLenght[1];
    fLenght[0] = fCodeStr.length();


    glShaderSource(_fShader, 1, fCode, fLenght);
    glCompileShader(_fShader);
    glGetShaderiv(_fShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(_fShader, sizeof(InfoLog), NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_FRAGMENT_SHADER, InfoLog);
        exit(1);
    }
    glAttachShader(_shaderProgram, _fShader);

    //--LINKING
    glLinkProgram(_shaderProgram);
    glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
    if (success == 0) {
        GLchar ErrorLog[1024];
        glGetProgramInfoLog(_shaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
    }
}


void Renderer3D::Normalize(float* data, int w, int h, int nbC){
    double* sum = (double*)calloc(nbC, sizeof(double));
    double* N = (double*)calloc(nbC, sizeof(double));
    
    for (int i = 0; i < w * h; i++) {
        for (int c = 0; c < nbC; c++) {
            int pixelid = c + i * nbC;
            sum[c] += data[pixelid];
            N[c] += data[pixelid];
        }
    }

    for (int c = 0; c < nbC; c++) {
        sum[c] = sum[c] / N[c];
    }

    for (int i = 0; i < w * h; i++) {
        for (int c = 0; c < nbC; c++) {
            int pixelid = c + i * nbC;
            int normalized = data[pixelid] - sum[c];
            normalized = std::min(255, std::max(0, normalized));
            data[pixelid] = normalized;
        }
    }
}



#endif