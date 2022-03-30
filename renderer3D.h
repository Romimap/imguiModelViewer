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
    GLuint _envMap = 0;
    GLuint _albedo = 0;
    GLuint _normal = 0;
    GLuint _roughness = 0;
    GLuint _bmap = 0;
    GLuint _mmap = 0;
    GLuint _constantSigma = 0;
    GLuint _var = 0;
    GLuint _mipchart = 0;

    glm::mat4x4 _projectionMatrix;
    glm::mat4x4 _viewMatrix;
    glm::mat4x4 _modelMatrix;

    std::vector<VertexData> _vertices;

    glm::vec3 *_cameraPosition;

    float s = 250;
    int mip_levels = 8;
    float max_aniso = 1;



public:
    Renderer3D(ImVec2 size, glm::vec3 &cameraPosition, const char* model, const char* vertexShader, const char* fragmentShader);
    ~Renderer3D();
    void Draw(ImVec2 size, ImVec4 clearColor, float dt, float t);
    std::string SetFShader(const std::string &code);
    std::string SetVShader(const std::string &code);

    void SetAlbedo(const char* path);
    void SetNormal(const char* path);

    glm::mat4 getProjectionMatrix() {return _projectionMatrix;}
    glm::mat4 getViewMatrix() {return _viewMatrix;}
    glm::mat4 getModelMatrix() {return _modelMatrix;}

    void Screenshot (const char* path);

private:
    void LoadMesh(const char* model);
    void MakeShaderProgram(const char* fragmentShader, const char* vertexShader);
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
    
    _projectionMatrix = glm::perspective<float>(glm::radians(55.0), _size.x / _size.y, 0.1f, 1000.0f);
    _modelMatrix = glm::mat4x4(1.0f);
    _viewMatrix = glm::lookAt(*_cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    {
        glGenTextures(1, &_roughness);
        glBindTexture(GL_TEXTURE_2D, _roughness);
        int w, h, nbC;
        unsigned char *data = stbi_load("textures/Gravel_Roughness.png", &w, &h, &nbC, 0); 
        glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_R8, w, h);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_R, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
    }
    {
        glGenTextures(1, &_envMap);
        glBindTexture(GL_TEXTURE_2D, _envMap);
        int w, h, nbC;
        unsigned char *data = stbi_load("textures/hdri_warehouse.png", &w, &h, &nbC, 0); 
        glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_RGB8, w, h);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
    }
    { // MIP CHART
        int mipw;
        int miph;
        int mipnbC;

        glGenTextures(1, &_mipchart);
        glBindTexture(GL_TEXTURE_2D, _mipchart);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 9);

        unsigned char *mip0 = stbi_load("textures/MIP/0.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, mip0);
        unsigned char *mip1 = stbi_load("textures/MIP/1.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 1, GL_RGB8, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, mip1);
        unsigned char *mip2 = stbi_load("textures/MIP/2.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 2, GL_RGB8, 64 , 64 , 0, GL_RGB, GL_UNSIGNED_BYTE, mip2);
        unsigned char *mip3 = stbi_load("textures/MIP/3.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 3, GL_RGB8, 32 , 32 , 0, GL_RGB, GL_UNSIGNED_BYTE, mip3);
        unsigned char *mip4 = stbi_load("textures/MIP/4.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 4, GL_RGB8, 16 , 16 , 0, GL_RGB, GL_UNSIGNED_BYTE, mip4);
        unsigned char *mip5 = stbi_load("textures/MIP/5.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 5, GL_RGB8, 8  , 8  , 0, GL_RGB, GL_UNSIGNED_BYTE, mip5);
        unsigned char *mip6 = stbi_load("textures/MIP/6.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 6, GL_RGB8, 4  , 4  , 0, GL_RGB, GL_UNSIGNED_BYTE, mip6);
        unsigned char *mip7 = stbi_load("textures/MIP/7.png", &mipw, &miph, &mipnbC, 0);
        glTexImage2D(GL_TEXTURE_2D, 7, GL_RGB8, 2  , 2  , 0, GL_RGB, GL_UNSIGNED_BYTE, mip7);
        glTexImage2D(GL_TEXTURE_2D, 8, GL_RGB8, 1  , 1  , 0, GL_RGB, GL_UNSIGNED_BYTE, mip0);
    }


    //Back to the default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


Renderer3D::~Renderer3D() {
    glDeleteFramebuffers(1, &_FBO);
    glDeleteTextures(1, &_outputColor);
}

void Renderer3D::SetAlbedo(const char* path) {
    glGenTextures(1, &_albedo);
    glBindTexture(GL_TEXTURE_2D, _albedo);
    int w, h, nbC;
    unsigned char *data = stbi_load(path, &w, &h, &nbC, 0);
    glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_RGB8, w, h);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
}

void Renderer3D::SetNormal(const char* path) {
    glGenTextures(1, &_normal);
    glBindTexture(GL_TEXTURE_2D, _normal);
    int w, h, nbC;
    unsigned char *data = stbi_load(path, &w, &h, &nbC, 0);
    glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_RGB8, w, h);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

    std::vector<std::vector<float>> dataB(64, std::vector<float>(0));
    std::vector<std::vector<float>> dataM(64, std::vector<float>(0));
    std::vector<std::vector<float>> dataS(64, std::vector<float>(0));
    std::vector<std::vector<float>> dataV(64, std::vector<float>(0));

    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            int pixelId = nbC * x + nbC * w * y;
            std::vector<float> pix(3);
            pix[0] = ((double)data[pixelId + 0] / 255.0) * 2.0 - 1.0; //Normal.x from [0;255] to [-1.0;1.0]
            pix[1] = ((double)data[pixelId + 1] / 255.0) * 2.0 - 1.0; //Normal.y from [0;255] to [-1.0;1.0]
            pix[2] = ((double)data[pixelId + 2] / 255.0) * 2.0 - 1.0; //Normal.z from [0;255] to [-1.0;1.0]

            std::vector<float> bbar(2); //NOTE: could be 0;2 / 1 if y was top
            bbar[0] = std::min(1.0f, std::max(-1.0f, pix[0] / (pix[2])));
            bbar[1] = std::min(1.0f, std::max(-1.0f, pix[1] / (pix[2])));

            dataB[0].push_back(bbar[0]);
            dataB[0].push_back(bbar[1]);
            dataB[0].push_back(pix[2]);

            dataM[0].push_back(bbar[0] * bbar[0] + (1.0/s));
            dataM[0].push_back(bbar[1] * bbar[1] + (1.0/s));
            dataM[0].push_back(bbar[0] * bbar[1]);

            dataS[0].push_back((1.0/s));
            dataS[0].push_back((1.0/s));
            dataS[0].push_back(0);

            dataV[0].push_back((1.0/s));
            dataV[0].push_back((1.0/s));
            dataV[0].push_back(0);
        }
    }

    glGenTextures(1, &_bmap);
    glBindTexture(GL_TEXTURE_2D, _bmap);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 9);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, dataB[0].data());

    glGenTextures(1, &_mmap);
    glBindTexture(GL_TEXTURE_2D, _mmap);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 9);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, dataM[0].data());

    glGenTextures(1, &_constantSigma);
    glBindTexture(GL_TEXTURE_2D, _constantSigma);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 9);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, dataS[0].data());

    glGenTextures(1, &_var);
    glBindTexture(GL_TEXTURE_2D, _var);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 9);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, dataV[0].data());


    //width and height of the mipmap levels
    int mw = w;
    int mh = h;
    for (int i = 1; mw > 0 && mh > 0; i++) { //For each mipmap level
        mw /= 2;
        mh /= 2;

        printf("%d, %d \n", mw, mh);

        std::vector<float> Sigma(3);
        Sigma[0] = 0.0f;
        Sigma[1] = 0.0f;
        Sigma[2] = 0.0f;
        float n = 0;

        for (int y = 0; y < mh; y++) {
            for (int x = 0; x < mw; x++) {
                std::vector<float> b(3);
                std::vector<float> m(3);
                int aID = nbC * (2 * x + 0) + nbC * (2 * mw) * (2 * y + 0);
                int bID = nbC * (2 * x + 1) + nbC * (2 * mw) * (2 * y + 0);
                int cID = nbC * (2 * x + 0) + nbC * (2 * mw) * (2 * y + 1);
                int dID = nbC * (2 * x + 1) + nbC * (2 * mw) * (2 * y + 1);

                b[0] = (dataB[i - 1][aID + 0] + dataB[i - 1][bID + 0] + dataB[i - 1][cID + 0] + dataB[i - 1][dID + 0]) / 4.0f;
                b[1] = (dataB[i - 1][aID + 1] + dataB[i - 1][bID + 1] + dataB[i - 1][cID + 1] + dataB[i - 1][dID + 1]) / 4.0f;
                b[2] = (dataB[i - 1][aID + 2] + dataB[i - 1][bID + 2] + dataB[i - 1][cID + 2] + dataB[i - 1][dID + 2]) / 4.0f;
                dataB[i].push_back(b[0]);
                dataB[i].push_back(b[1]);
                dataB[i].push_back(b[2]);

                m[0] = (dataM[i - 1][aID + 0] + dataM[i - 1][bID + 0] + dataM[i - 1][cID + 0] + dataM[i - 1][dID + 0]) / 4.0f;
                m[1] = (dataM[i - 1][aID + 1] + dataM[i - 1][bID + 1] + dataM[i - 1][cID + 1] + dataM[i - 1][dID + 1]) / 4.0f;
                m[2] = (dataM[i - 1][aID + 2] + dataM[i - 1][bID + 2] + dataM[i - 1][cID + 2] + dataM[i - 1][dID + 2]) / 4.0f;
                dataM[i].push_back(m[0]);
                dataM[i].push_back(m[1]);
                dataM[i].push_back(m[2]);

                Sigma[0] += m[0] - b[0] * b[0]; //Vx
                Sigma[1] += m[1] - b[1] * b[1]; //Vy
                Sigma[2] += m[2] - b[0] * b[1]; //Cxy

                int footprintSize = pow(2, i);

                //For each u in the footprint
                std::vector<float> v(3);
                int startX = x * footprintSize;
                int startY = y * footprintSize;
                for (int j = startX; j < footprintSize + startX; j++) {
                    for (int k = startY; k < footprintSize + startY; k++) {
                        int uid = nbC * j + nbC * w * k;

                        v[0] += (pow(dataB[0][uid + 0] - b[0], 2));
                        v[1] += (pow(dataB[0][uid + 1] - b[1], 2));
                        v[2] += (pow(dataB[0][uid + 2] - b[2], 2));
                    }
                }
                v[0] /= pow(footprintSize, 2);
                v[1] /= pow(footprintSize, 2);
                v[2] /= pow(footprintSize, 2);

                dataV[i].push_back(v[0] + (1.0/s));
                dataV[i].push_back(v[1] + (1.0/s));
                dataV[i].push_back(v[2]);

                n++;
            }
        }

        Sigma[0] /= n;
        Sigma[1] /= n;
        Sigma[2] /= n;

        printf("Sigma: %f, %f, %f \n", Sigma[0], Sigma[1], Sigma[2]);

        for (int j = 0; j < mh * mw * 3; j++) {
            dataS[i].push_back(Sigma[0]);
            dataS[i].push_back(Sigma[1]);
            dataS[i].push_back(Sigma[2]);
        }

        glBindTexture(GL_TEXTURE_2D, _bmap);
        glTexImage2D(GL_TEXTURE_2D, i, GL_RGB32F, mw, mh, 0, GL_RGB, GL_FLOAT, dataB[i].data());
        glBindTexture(GL_TEXTURE_2D, _mmap);
        glTexImage2D(GL_TEXTURE_2D, i, GL_RGB32F, mw, mh, 0, GL_RGB, GL_FLOAT, dataM[i].data());
        glBindTexture(GL_TEXTURE_2D, _constantSigma);
        glTexImage2D(GL_TEXTURE_2D, i, GL_RGB32F, mw, mh, 0, GL_RGB, GL_FLOAT, dataS[i].data());
        glBindTexture(GL_TEXTURE_2D, _var);
        glTexImage2D(GL_TEXTURE_2D, i, GL_RGB32F, mw, mh, 0, GL_RGB, GL_FLOAT, dataV[i].data());
    }
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

void Renderer3D::Draw(ImVec2 size, ImVec4 clearColor, float dt, float t) {
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO); //Bind
    if (_size.x != size.x || _size.y != size.y) {
        glDeleteTextures(1, &_outputColor);
        _size = size;
        //Create & Attach a texture to it
        glGenTextures(1, &_outputColor);
        glBindTexture(GL_TEXTURE_2D, _outputColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _size.x, _size.y, 0,  GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outputColor, 0);

        glGenTextures(1, &_outputDepth);
        glBindTexture(GL_TEXTURE_2D, _outputDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _size.x, _size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _outputDepth, 0);
        
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
    glBindTexture(GL_TEXTURE_2D, _albedo);
    glUniform1i(glGetUniformLocation(_shaderProgram, "albedo"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _normal);
    glUniform1i(glGetUniformLocation(_shaderProgram, "normal"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _roughness);
    glUniform1i(glGetUniformLocation(_shaderProgram, "roughness"), 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _bmap);
    glUniform1i(glGetUniformLocation(_shaderProgram, "bmap"), 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, _mmap);
    glUniform1i(glGetUniformLocation(_shaderProgram, "mmap"), 4);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, _mipchart);
    glUniform1i(glGetUniformLocation(_shaderProgram, "mipchart"), 5);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, _constantSigma);
    glUniform1i(glGetUniformLocation(_shaderProgram, "constantSigma"), 6);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, _var);
    glUniform1i(glGetUniformLocation(_shaderProgram, "var"), 7);

    glDrawArrays(GL_TRIANGLES, 0, _vertices.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); //Unbind

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


#endif