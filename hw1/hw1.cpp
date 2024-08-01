/*
  CSCI 420 Computer Graphics, Computer Science, USC
  Assignment 2: Rollercoaster.
  C/C++ starter code
  Student name: Mahesh Joseph Sadashiv
  Student username: sadashiv
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"
#include <fstream>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <cmath>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openGLHeader.h"
#include "imageIO.h"



#define FRAME_RATE 50

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum {
    ROTATE, TRANSLATE, SCALE
} CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

typedef enum {
    PAUSE, UNPAUSE
} MODE_STATE;
MODE_STATE modeState = UNPAUSE;
// Transformations of the terrain.
float terrainRotate[3] = {0.0f, 0.0f, 0.0f};
float terrainTranslate[3] = {0.0f, 0.0f, 0.0f};
float terrainScale[3] = {1.0f, 1.0f, 1.0f};

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 2";


// Number of vertices in the single triangle (starter code).
int numVertices;
// Number of lines.
int numSplineVertices;
// Number of triangles verticies
int numTriangleVertices;
//width and height of the image
int width, height;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram *pipelineProgram = nullptr;
//create pipeline program for texture and other texture variables
PipelineProgram *pipelineTextureProgram = nullptr;
GLuint textHandle;
VBO *vboTexturePlane = nullptr;
VBO *vboTextureMap = nullptr;
VAO *vaoTexture = nullptr;
int numTextureVertices;
//create pipeline program for texture and other texture variables
PipelineProgram *pipelineSkyTextureProgram = nullptr;
GLuint nightTextureHandle;
VBO *vboSkyTexturePlane = nullptr;
VBO *vboSkyTextureMap = nullptr;
VAO *vaoSkyTexture = nullptr;
int numSkyTextureVertices;

float MIN_Y = FLT_MAX;
float MAX_Y = -FLT_MAX;

// VBO for rails
VBO *vboRails = nullptr;
VBO *vboNormals = nullptr;
VAO *vaoRails = nullptr;
// VBO for Crosstie
VBO *vboCrosstie = nullptr;
VBO *vboCrosstieNormals = nullptr;
VAO *vaoCrosstieRails = nullptr;
int numCrosstieVertices = 0;

typedef enum {
    RIGHT, LEFT
} RailPosition;

//camera init
float eye[3] = {0.0f, 1.5f, 1.5f};
float lookingAt[3] = {0.0f, 0.0f, 0.0f};
float up[3] = {0.0f, 1.0f, 0.0f};
//scale and exponent for mode 4
float scale = 1;
float exponent = 1;
int numImages = 0;

//Catmull-Rom Spline variables
float s = 0.5;

// Represents one spline control point.
struct Point {
    double x, y, z;
};

// Contains the control points of the spline.
struct Spline {
    int numControlPoints;
    Point *points;
};
Spline * splines;
int numSplines;
// interopolated points on the spline
vector<glm::vec3> pointsOnSpline;
// tangents at each point on the spline
vector<glm::vec3> tangentsOnSpline;
vector<glm::vec3> normalsOnSpline;
//normals for color
vector<glm::vec3> binormalsOnSpline;
//spline iterater
int splineIterater = 0;
//lighting
float K_d = 0.75;
float L_d = 0.95;
glm::vec3 l_d = glm::vec3(1, 2, 2);
//delay till next call back in milli seconds:
int waitBeforeMove = 1;

//basis matrix
glm::mat4 basis = glm::mat4(
        -s,   2*s, -s, 0,
        2-s,   s-3,  0, 1,
        s-2, 3-2*s,  s, 0,
        s,      -s,  0, 0
);
//points between control points
int pointPerInterval = 1000;
//number of vertices for rails
int numRailVertices = 0;

void loadSpline(char *argv) {
    vector<string> splineFiles;
    string myText;
    ifstream MyReadFile("splines.txt");

    while (getline (MyReadFile, myText)) {
        splineFiles.push_back(myText);
    }
    numSplines = splineFiles.size();
    MyReadFile.close();
    splines = (Spline*) malloc(numSplines * sizeof(Spline));
    cout<<"number of splines : "<<numSplines<<endl;
    for(int j = 0;j < numSplines; j++){
        FILE *fileSpline = fopen(splineFiles[j].c_str(), "r");
        if (fileSpline == NULL) {
            printf("Cannot open file %s.\n", argv);
            exit(1);
        }

        // Read the number of spline control points.
        fscanf(fileSpline, "%d\n", &splines[j].numControlPoints);
        printf("Detected %d control points.\n", splines[j].numControlPoints);

        // Allocate memory.
        splines[j].points = (Point *) malloc(splines[j].numControlPoints * sizeof(Point));
        float curMax = -FLT_MAX;
        // Load the control points.
        for (int i = 0; i < splines[j].numControlPoints; i++) {
            if (fscanf(fileSpline, "%lf %lf %lf",
                       &splines[j].points[i].x,
                       &splines[j].points[i].y,
                       &splines[j].points[i].z) != 3) {
                printf("Error: incorrect number of control points in file %s.\n", argv);
                exit(1);
            }
            float m = std::max(std::max((float)splines[j].points[i].x, (float)splines[j].points[i].y), (float)splines[j].points[i].z);
            curMax = std::max(curMax, m);
        }
        if (curMax > 1) {
            for (int k = 0; k < splines[j].numControlPoints; k++) {
                splines[j].points[k].x /= curMax;
                splines[j].points[k].y /= curMax;
                splines[j].points[k].z /= curMax;
            }
        }
    }
}

int initTexture(const char *imageFilename, GLuint textureHandle) {
    // Read the texture image.
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);

    if (err != ImageIO::OK) {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }

    // Check that the number of bytes is a multiple of 4.
    if (img.getWidth() * img.getBytesPerPixel() % 4) {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }

    // Allocate space for an array of pixels.
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char *pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

    // Fill the pixelsRGBA array with the image pixels.
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++) {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c <
                            numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }

    // Bind the texture.
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    // Initialize the texture.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

    // Generate the mipmaps for this texture.
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set the texture parameters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Query support for anisotropic texture filtering.
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // Set anisotropic texture filtering.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

    // Query for any errors.
    GLenum errCode = glGetError();
    if (errCode != 0) {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }

    // De-allocate the pixel array -- it is no longer needed.
    delete[] pixelsRGBA;

    return 0;
}


void initSkyTexture() {
    float scale = 1.5;
    vector<float> vertices = {
            // positions
            1,MIN_Y,1,
            1,MAX_Y,1,
            -1,MIN_Y,1,
            1,MAX_Y,1,
            -1,MIN_Y,1,
            -1,MAX_Y,1,

            -1,MIN_Y,1,
            -1,MAX_Y,1,
            -1,MIN_Y,-1,
            -1, MAX_Y,1,
            -1,MIN_Y,-1,
            -1,MAX_Y,-1,

            -1,MIN_Y,-1,
            -1,MAX_Y,-1,
            1,MIN_Y,-1,
            -1,MAX_Y,-1,
            1,MIN_Y,-1,
            1,MAX_Y,-1,

            1, MIN_Y, -1,
            1,MAX_Y,-1,
            1, MIN_Y,1,
            1,MAX_Y,-1,
            1, MIN_Y,1,
            1,MAX_Y,1,

            1,MAX_Y,1,
            1,MAX_Y,-1,
            -1,MAX_Y,1,
            1,MAX_Y,-1,
            -1,MAX_Y,1,
            -1,MAX_Y,-1
    };

    for (int i = 0; i < vertices.size(); i++) {
        vertices[i] *= scale;
    }
    vector<float>uv;
    vector<float> u = {
             1,0
            ,1,1
            ,0,0
            ,1,1
            ,0,0
            ,0,1};
    for(int i = 0;i<5;i++){
        uv.insert(uv.end(), u.begin(), u.end());
    }

    numSkyTextureVertices = vertices.size()/3;
    vboSkyTexturePlane = new VBO(numSkyTextureVertices, 3, vertices.data(), GL_STATIC_DRAW);
    vboSkyTextureMap = new VBO(numSkyTextureVertices, 2, uv.data(), GL_STATIC_DRAW);
    vaoSkyTexture = new VAO();
    vaoSkyTexture->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineSkyTextureProgram, vboSkyTexturePlane, "position");
    vaoSkyTexture->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineSkyTextureProgram, vboSkyTextureMap, "texCoord");
}

void initSpline() {
    for (int i = 0; i<numSplines; i++){
        for (int j = 1; j < splines[i].numControlPoints - 2; j++) {
            glm::mat3x4 control =glm::mat3x4(
                    splines[i].points[j-1].x, splines[i].points[j].x, splines[i].points[j+1].x, splines[i].points[j+2].x,
                    splines[i].points[j-1].y, splines[i].points[j].y, splines[i].points[j+1].y, splines[i].points[j+2].y,
                    splines[i].points[j-1].z, splines[i].points[j].z, splines[i].points[j+1].z, splines[i].points[j+2].z
            );
            for (int k = 0; k<pointPerInterval; k++)
            {
                float u = k*(1.0/(pointPerInterval-1));
                glm::vec4 uu(u*u*u, u*u, u, 1);
                glm::vec3 point =  glm::transpose(control) * glm::transpose(basis) * uu;
                pointsOnSpline.push_back(point);
            }
        }
    }

    numSplineVertices = pointsOnSpline.size();
}

void createTangents(){
    for (int i = 0; i<numSplines; i++) {
        for (int j = 1; j < splines[i].numControlPoints - 2; j++) {
            glm::mat3x4 control = glm::mat3x4(
                    splines[i].points[j - 1].x, splines[i].points[j].x, splines[i].points[j + 1].x, splines[i].points[j + 2].x,
                    splines[i].points[j - 1].y, splines[i].points[j].y, splines[i].points[j + 1].y, splines[i].points[j + 2].y,
                    splines[i].points[j - 1].z, splines[i].points[j].z, splines[i].points[j + 1].z, splines[i].points[j + 2].z
            );
            for (int k = 0; k < pointPerInterval; k++) {
                float u = k * (1.0 / (pointPerInterval - 1));
                glm::vec4 uu(3 * u * u, 2 * u, 1, 0);
                glm::vec3 tan = glm::transpose(control) * glm::transpose(basis) * uu;
                tangentsOnSpline.push_back(tan);
            }
        }
    }
}

void calculateNormalToTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, vector<float> &normals){
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
    for(int i = 0; i < 3; i++){
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
    }
}

void createNormals() {
    glm::vec3 initial(0.0, 0.0, -1.0);
    glm::vec3 n0 = glm::cross(tangentsOnSpline[0], initial);
    glm::vec3 b0 = glm::normalize(glm::cross(tangentsOnSpline[0], n0));
    normalsOnSpline.push_back(n0);
    binormalsOnSpline.push_back(b0);
    int k = 0;
    for (int i = 1; i < pointsOnSpline.size(); i++) {
        glm::vec3 n = glm::normalize(glm::cross(binormalsOnSpline[i - 1], tangentsOnSpline[i]));
        glm::vec3 b = glm::normalize(glm::cross(tangentsOnSpline[i], n));
        normalsOnSpline.push_back(n);
        binormalsOnSpline.push_back(b);
    }
}

void createRails(vector<float> &positions, vector<glm::vec3> &points, RailPosition railPosition, vector<float> &normals){
    for(int j = 0; j < 4; j++) {
        for (int i = 0; i < pointsOnSpline.size() - 1; i++) {
            positions.push_back(points[i * 4 + j].x );
            positions.push_back(points[i * 4 + j].y);
            positions.push_back(points[i * 4 + j].z);

            MIN_Y = std::min(points[i * 4 + j].y, MIN_Y);
            MIN_Y = std::min(points[i * 4 + (3 + j) % 4].y, MIN_Y);

            MAX_Y = std::max(points[i * 4 + j].y, MAX_Y);
            MAX_Y = std::max(points[i * 4 + (3 + j) % 4].y, MAX_Y);

            positions.push_back(points[i * 4 + (3 + j) % 4].x );
            positions.push_back(points[i * 4 + (3 + j) % 4].y);
            positions.push_back(points[i * 4 + (3 + j) % 4].z);

            positions.push_back(points[(i + 1) * 4 + j].x );
            positions.push_back(points[(i + 1) * 4 + j].y);
            positions.push_back(points[(i + 1) * 4 + j].z);

            calculateNormalToTriangle(points[i * 4 + j], points[i * 4 + (3 + j) % 4],
                                      points[(i + 1) * 4 + j], normals);

            positions.push_back(points[(i + 1) * 4 + j].x );
            positions.push_back(points[(i + 1) * 4 + j].y);
            positions.push_back(points[(i + 1) * 4 + j].z);

            positions.push_back(points[(i + 1) * 4 + (3 + j) % 4].x );
            positions.push_back(points[(i + 1) * 4 + (3 + j) % 4].y);
            positions.push_back(points[(i + 1) * 4 + (3 + j) % 4].z);

            positions.push_back(points[i * 4 + (3 + j) % 4].x );
            positions.push_back(points[i * 4 + (3 + j) % 4].y);
            positions.push_back(points[i * 4 + (3 + j) % 4].z);

            calculateNormalToTriangle(points[(i + 1) * 4 + j], points[i * 4 + (3 + j) % 4],
                                      points[(i + 1) * 4 + (3 + j) % 4], normals);
        }
    }
}

void generateCuboidVertices(float width, float height, float depth,
                                              const glm::vec3& center, const glm::vec3 normal,
                                              const glm::vec3& binormal, const glm::vec3 tangent,
                                              vector<float> &positions, vector<float> &normals
                                              ) {
    // Calculate half extents
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    float halfDepth = depth / 2.0f;

    // Calculate basis vectors for the local coordinate system of the cuboid
    glm::vec3 u, v, w;
    w = glm::normalize(normal);
    u = glm::normalize(binormal);
    v = glm::normalize(tangent);

    // Calculate cuboid vertices
    std::vector<glm::vec3> vertices;

    // Generate vertices by combining the center point with the basis vectors
    glm::vec3 offsetU = u * halfWidth;
    glm::vec3 offsetV = v * halfHeight;
    glm::vec3 offsetW = w * halfDepth;


    vertices.push_back(center + offsetU - offsetV - offsetW); // Front bottom right
    vertices.push_back(center + offsetU + offsetV - offsetW); // Front top right
    vertices.push_back(center - offsetU + offsetV - offsetW); // Front top left
    vertices.push_back(center - offsetU - offsetV - offsetW); // Front bottom left


    vertices.push_back(center + offsetU - offsetV + offsetW); // Back bottom right
    vertices.push_back(center + offsetU + offsetV + offsetW); // Back top right
    vertices.push_back(center - offsetU + offsetV + offsetW); // Back top left
    vertices.push_back(center - offsetU - offsetV + offsetW); // Back bottom left


    for(int j = 0; j < 4; j++) {
        for (int i = 0; i < 1; i++) {
            positions.push_back(vertices[i * 4 + j].x);
            positions.push_back(vertices[i * 4 + j].y);
            positions.push_back(vertices[i * 4 + j].z);

            positions.push_back(vertices[i * 4 + (3 + j) % 4].x);
            positions.push_back(vertices[i * 4 + (3 + j) % 4].y);
            positions.push_back(vertices[i * 4 + (3 + j) % 4].z);

            positions.push_back(vertices[(i + 1) * 4 + j].x);
            positions.push_back(vertices[(i + 1) * 4 + j].y);
            positions.push_back(vertices[(i + 1) * 4 + j].z);

            calculateNormalToTriangle(vertices[i * 4 + j], vertices[i * 4 + (3 + j) % 4],
                                      vertices[(i + 1) * 4 + j], normals);

            positions.push_back(vertices[(i + 1) * 4 + j].x);
            positions.push_back(vertices[(i + 1) * 4 + j].y);
            positions.push_back(vertices[(i + 1) * 4 + j].z);

            positions.push_back(vertices[(i + 1) * 4 + (3 + j) % 4].x);
            positions.push_back(vertices[(i + 1) * 4 + (3 + j) % 4].y);
            positions.push_back(vertices[(i + 1) * 4 + (3 + j) % 4].z);

            positions.push_back(vertices[i * 4 + (3 + j) % 4].x);
            positions.push_back(vertices[i * 4 + (3 + j) % 4].y);
            positions.push_back(vertices[i * 4 + (3 + j) % 4].z);

            calculateNormalToTriangle(vertices[(i + 1) * 4 + j], vertices[i * 4 + (3 + j) % 4],
                                      vertices[(i + 1) * 4 + (3 + j) % 4], normals);
        }
    }
    positions.push_back(vertices[4].x);
    positions.push_back(vertices[4].y);
    positions.push_back(vertices[4].z);

    positions.push_back(vertices[7].x);
    positions.push_back(vertices[7].y);
    positions.push_back(vertices[7].z);

    positions.push_back(vertices[5].x);
    positions.push_back(vertices[5].y);
    positions.push_back(vertices[5].z);

    calculateNormalToTriangle(vertices[4], vertices[7], vertices[5], normals);

    positions.push_back(vertices[7].x);
    positions.push_back(vertices[7].y);
    positions.push_back(vertices[7].z);

    positions.push_back(vertices[5].x);
    positions.push_back(vertices[5].y);
    positions.push_back(vertices[5].z);

    positions.push_back(vertices[6].x);
    positions.push_back(vertices[6].y);
    positions.push_back(vertices[6].z);

    calculateNormalToTriangle(vertices[7], vertices[5], vertices[6], normals);

    positions.push_back(vertices[0].x);
    positions.push_back(vertices[0].y);
    positions.push_back(vertices[0].z);

    positions.push_back(vertices[3].x);
    positions.push_back(vertices[3].y);
    positions.push_back(vertices[3].z);

    positions.push_back(vertices[1].x);
    positions.push_back(vertices[1].y);
    positions.push_back(vertices[1].z);

    calculateNormalToTriangle(vertices[0], vertices[3], vertices[1], normals);

    positions.push_back(vertices[3].x);
    positions.push_back(vertices[3].y);
    positions.push_back(vertices[3].z);

    positions.push_back(vertices[1].x);
    positions.push_back(vertices[1].y);
    positions.push_back(vertices[1].z);

    positions.push_back(vertices[2].x);
    positions.push_back(vertices[2].y);
    positions.push_back(vertices[2].z);

    calculateNormalToTriangle(vertices[3], vertices[1], vertices[2], normals);
}


void createCrosstie(){
    vector<float> positions;
    vector<float> normals;
    for(int i = 0;i<pointsOnSpline.size();i+=100){
        generateCuboidVertices(0.01,0.003,0.003,pointsOnSpline[i],normalsOnSpline[i],
                               binormalsOnSpline[i], tangentsOnSpline[i],positions,normals);
    }
    numCrosstieVertices = positions.size()/3;
    vboCrosstie = new VBO(numCrosstieVertices, 3, positions.data(), GL_STATIC_DRAW);
    vboCrosstieNormals = new VBO(numCrosstieVertices, 3, normals.data(), GL_STATIC_DRAW);
    vaoCrosstieRails = new VAO();
    vaoCrosstieRails->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboCrosstie, "position");
    vaoCrosstieRails->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboCrosstieNormals, "normals");
}

void createTrack(){
    vector<float> positions;
    vector<float> normals;
    float alpha = 0.004;
    float beta = 0.9;
    float gama = 0.001;
    vector<glm::vec3> pointsLeft;
    vector<glm::vec3> pointsRight;
    for(int i = 0; i < pointsOnSpline.size(); i++){
        glm::vec3 b_0 = pointsOnSpline[i] + alpha * (-normalsOnSpline[i] + binormalsOnSpline[i]);
        glm::vec3 b_1 = pointsOnSpline[i] + alpha * (normalsOnSpline[i] + binormalsOnSpline[i]);
        glm::vec3 b_2 = pointsOnSpline[i] + alpha * (normalsOnSpline[i] - binormalsOnSpline[i]);
        glm::vec3 b_3 = pointsOnSpline[i] + alpha * (-normalsOnSpline[i] - binormalsOnSpline[i]);

        glm::vec3 b_0_1 = b_3 + beta * (b_0 - b_3);
        glm::vec3 b_1_1 = b_2 + beta * (b_1 - b_2);
        glm::vec3 b_2_1 = b_1 + beta * (b_2 - b_1);
        glm::vec3 b_3_1 = b_0 + beta * (b_3 - b_0);

        pointsLeft.push_back(b_3);
        pointsLeft.push_back(b_2);
        pointsLeft.push_back(b_2_1);
        pointsLeft.push_back(b_3_1);

        pointsRight.push_back(b_0_1);
        pointsRight.push_back(b_1_1);
        pointsRight.push_back(b_1);
        pointsRight.push_back(b_0);
    }
    createRails(positions, pointsLeft, LEFT, normals);
    createRails(positions, pointsRight, RIGHT, normals);

    numRailVertices = positions.size()/3;
    cout<<"numRailVertices : " << numRailVertices << "normals : "  << normals.size()/3 << endl;
    vboRails = new VBO(numRailVertices, 3, positions.data(), GL_STATIC_DRAW);
    vboNormals = new VBO(numRailVertices, 3, normals.data(), GL_STATIC_DRAW);
    vaoRails = new VAO();
    vaoRails->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboRails, "position");
    vaoRails->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboNormals, "normals");
}

void setLightingVariables(){
    float l_d_f[] = {l_d.x, l_d.y, l_d.z};
    pipelineProgram->SetUniformVariableVec3("l_d", l_d_f);
    pipelineProgram->SetUniformVariablef("L_d", L_d);
    pipelineProgram->SetUniformVariablef("K_d", K_d);
}

void renderTrack(){
    setLightingVariables();
    vaoRails->Bind();
    glDrawArrays(GL_TRIANGLES, 0, numRailVertices);
    vaoCrosstieRails->Bind();
    glDrawArrays(GL_TRIANGLES, 0, numCrosstieVertices);
}

//creare texture coordinates
void createTexture(){
    float scale = 3.5f;
    numTextureVertices = 24;
    vector<float> planePositions = {
         0,MIN_Y,1,
         0,MIN_Y,0,
         -1,MIN_Y,1,

        -1,MIN_Y,1,
        0,MIN_Y,0,
        -1,MIN_Y,0,

        1,MIN_Y,1,
        1,MIN_Y,0,
        0,MIN_Y,1,

        0,MIN_Y,1,
        1,MIN_Y,0,
        0,MIN_Y,0,

        1,MIN_Y,0,
        1,MIN_Y,-1,
        0,MIN_Y,0,

        0,MIN_Y,0,
        1,MIN_Y,-1,
        0,MIN_Y,-1,

        0,MIN_Y,0,
        0,MIN_Y,-1,
        -1,MIN_Y,0,

        0,MIN_Y,-1,
        -1,MIN_Y,0,
        -1,MIN_Y,-1
    };
    for(int i = 0; i < 3 * numTextureVertices; i++){
        planePositions[i] *= scale;
    }

    vector<float> uv = {
            1,0
            ,1,1
            ,0,0

            ,0,0
            ,1,1
            ,0,1,

            1,0
            ,1,1
            ,0,0

            ,0,0
            ,1,1
            ,0,1,

            1,0
            ,1,1
            ,0,0

            ,0,0
            ,1,1
            ,0,1,

            1,0
            ,1,1
            ,0,0

            ,0,0
            ,1,1
            ,0,1,

    };

    vboTexturePlane = new VBO(numTextureVertices, 3, planePositions.data(), GL_STATIC_DRAW);
    vboTextureMap = new VBO(numTextureVertices, 2, uv.data(), GL_STATIC_DRAW);
    vaoTexture = new VAO();
    vaoTexture->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineTextureProgram, vboTexturePlane, "position");
    vaoTexture->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineTextureProgram, vboTextureMap, "texCoord");
    uv.clear();
    uv.shrink_to_fit();
    planePositions.clear();
    planePositions.shrink_to_fit();
}

// Write a screenshot to the specified filename.
void saveScreenshot(const char *filename) {
    unsigned char *screenshotData = new unsigned char[windowWidth * windowHeight * 3];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

    ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

    if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
        cout << "File " << filename << " saved successfully." << endl;
    else cout << "Failed to save file " << filename << '.' << endl;

    delete[] screenshotData;
}

void setCameraPosition(){
    if (splineIterater < numSplineVertices) {
        eye[0] = pointsOnSpline[splineIterater].x  + (0.01) * normalsOnSpline[splineIterater].x;
        eye[1] = pointsOnSpline[splineIterater].y  +  (0.01) * normalsOnSpline[splineIterater].y;
        eye[2] = pointsOnSpline[splineIterater].z  + (0.01) * normalsOnSpline[splineIterater].z;

        lookingAt[0] = pointsOnSpline[splineIterater].x + tangentsOnSpline[splineIterater].x;
        lookingAt[1] = pointsOnSpline[splineIterater].y + tangentsOnSpline[splineIterater].y;
        lookingAt[2] = pointsOnSpline[splineIterater].z + tangentsOnSpline[splineIterater].z;

        up[0] = normalsOnSpline[splineIterater].x;
        up[1] = normalsOnSpline[splineIterater].y;
        up[2] = normalsOnSpline[splineIterater].z;
    } else {
        splineIterater = 0;
    }
}

void idleFunc(int val) {
    // Do some stuff...
    // For example, here, you can save the screenshots to disk (to make the animation).

    // Notify GLUT that it should call displayFunc.

    glutPostRedisplay();
}

void reshapeFunc(int w, int h) {
    glViewport(0, 0, w, h);

    // When the window has been resized, we need to re-set our projection matrix.
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.LoadIdentity();
    // You need to be careful about setting the zNear and zFar.
    // Anything closer than zNear, or further than zFar, will be culled.
    const float zNear = 0.001f;
    const float zFar = 1000.0f;
    const float humanFieldOfView = 60.0f;
    matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y) {
    // Mouse has moved, and one of the mouse buttons is pressed (dragging).

    // the change in mouse position since the last invocation of this function
    int mousePosDelta[2] = {x - mousePos[0], y - mousePos[1]};

    switch (controlState) {
        // translate the terrain
        case TRANSLATE:
            if (rightMouseButton) {
                // control x,y translation via the left mouse button
                terrainTranslate[0] += mousePosDelta[0] * 0.01f;
                terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
            }
            if (middleMouseButton) {
                // control z translation via the middle mouse button
                terrainTranslate[2] += mousePosDelta[1] * 0.01f;
            }
            break;

            // rotate the terrain
        case ROTATE:
            if (leftMouseButton) {
                // control x,y rotation via the left mouse button
                terrainRotate[0] += mousePosDelta[1];
                terrainRotate[1] += mousePosDelta[0];
            }
            if (middleMouseButton) {
                // control z rotation via the middle mouse button
                terrainRotate[2] += mousePosDelta[1];
            }
            break;

            // scale the terrain
        case SCALE:
            if (leftMouseButton) {
                // control x,y scaling via the left mouse button
                terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
                terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
            }
            if (middleMouseButton) {
                // control z scaling via the middle mouse button
                terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
            }
            break;
    }

    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseMotionFunc(int x, int y) {
    // Mouse has moved.
    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y) {
    // A mouse button has has been pressed or depressed.

    // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
    switch (button) {
        case GLUT_LEFT_BUTTON:
            leftMouseButton = (state == GLUT_DOWN);
            break;

        case GLUT_MIDDLE_BUTTON:
            middleMouseButton = (state == GLUT_DOWN);
            break;

        case GLUT_RIGHT_BUTTON:
            rightMouseButton = (state == GLUT_DOWN);
            break;
    }

    // Keep track of whether CTRL and SHIFT keys are pressed.
    switch (glutGetModifiers()) {
        case GLUT_ACTIVE_CTRL:
            controlState = TRANSLATE;
            break;

        case GLUT_ACTIVE_SHIFT:
            controlState = SCALE;
            break;

            // If CTRL and SHIFT are not pressed, we are in rotate mode.
        default:
            controlState = ROTATE;
            break;
    }

    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y) {
    ostringstream oss;
    string with3digits;
    switch (key) {
        case 27: // ESC key
            exit(0); // exit the program
            break;

        case ' ':
            if(modeState == PAUSE){
                modeState = UNPAUSE;
            }else{
                modeState = PAUSE;
            }
            break;

        case 'x':
            // Take a screenshot.
            oss << std::setw(3) << std::setfill('0') << numImages;
            with3digits = "./outputImages/" + oss.str() + ".jpg";
            cout << with3digits << endl;
            numImages++;
            saveScreenshot(with3digits.c_str());
            break;
    }
}

void displayFunc() {
    // This function performs the actual rendering.

    // First, clear the screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setCameraPosition();
    // Set up the camera position, focus point, and the up vector.
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    matrix.LookAt(eye[0], eye[1], eye[2],
                  lookingAt[0], lookingAt[1], lookingAt[2],
                  up[0], up[1], up[2]);
    matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
    matrix.Rotate(terrainRotate[0], 1, 0, 0);
    matrix.Rotate(terrainRotate[1], 0, 1, 0);
    matrix.Rotate(terrainRotate[2], 0, 0, 1);
    matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

    // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
    // ...

    // Read the current modelview and projection matrices from our helper class.
    // The matrices are only read here; nothing is actually communicated to OpenGL yet.
    float modelViewMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetMatrix(modelViewMatrix);

    float projectionMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(projectionMatrix);


    pipelineTextureProgram->Bind();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textHandle);
    pipelineTextureProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    pipelineTextureProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
    vaoTexture->Bind();
    glDrawArrays(GL_TRIANGLES, 0, numTextureVertices);
    glDisable(GL_TEXTURE_2D);

    pipelineSkyTextureProgram->Bind();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, nightTextureHandle);
    pipelineSkyTextureProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    pipelineSkyTextureProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
    vaoSkyTexture->Bind();
    glDrawArrays(GL_TRIANGLES, 0, numSkyTextureVertices);
    glDisable(GL_TEXTURE_2D);

    // Execute the rendering.
    // Bind the VAO that we want to render. Remember, one object = one VAO.
    pipelineProgram->Bind();
    pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
    renderTrack();

    // Swap the double-buffers.
    glutSwapBuffers();
}

int calculateSplineIterator(int splineIterater){
    int newSplineIterator = lround(splineIterater + waitBeforeMove * 5 * sqrt(2 * 9.8 * (MAX_Y + pointsOnSpline[splineIterater].y)/pointsOnSpline[splineIterater].length()));
    if (newSplineIterator < pointsOnSpline.size()){
        if(newSplineIterator == splineIterater){
            newSplineIterator++;
        }
        return newSplineIterator;
    }
    return 0;
}

void timerFunc(int t)
{
    if (splineIterater == pointsOnSpline.size()-1) {
        splineIterater = 0;
    }
    if(modeState == UNPAUSE){
        splineIterater = calculateSplineIterator(splineIterater);
    }

    glutTimerFunc(waitBeforeMove, timerFunc, splineIterater);
    glutPostRedisplay();
}

void initScene(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <spline file>\n", argv[0]);
        exit(0);
    }

    // Load spline from the provided filename.
    loadSpline(argv[1]);

    //printf("Loaded spline with %d control point(s).\n", spline.numControlPoints);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    if (pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0) {
        cout << "Failed to build the pipeline program." << endl;
        throw 1;
    }
    cout << "Successfully built the pipeline program." << endl;
    pipelineProgram->Bind();

    //init texture
    glGenTextures(1, &textHandle);
    if (initTexture("./textures/groundTexture.jpg", textHandle) != 0) {
        printf("Texture initialization error.");
    }
    pipelineTextureProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    if (pipelineTextureProgram->BuildShadersFromFiles(shaderBasePath, "vertexTextureShader.glsl", "fragmentTextureShader.glsl") != 0) {
        cout << "Failed to build the pipeline program." << endl;
        throw 1;
    }
    cout << "Successfully built the texture pipeline program." << endl;

    //init texture
    glGenTextures(1, &nightTextureHandle);
    if (initTexture("./textures/skybox/left.jpg", nightTextureHandle) != 0) {
        printf("Texture night initialization error.");
    }
    pipelineSkyTextureProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    if (pipelineSkyTextureProgram->BuildShadersFromFiles(shaderBasePath, "vertexSkyTextureShader.glsl", "fragmentSkyTextureShader.glsl") != 0) {
        cout << "Failed to build the pipeline program." << endl;
        throw 1;
    }
    cout << "Successfully built the texture pipeline program." << endl;

    // create the vbo and vao for lines
    initSpline();

    //create tangents, normals and binormals
    createTangents();
    createNormals();

    //create the rail vbo and vao
    createTrack();
    //create the cross tie
    createCrosstie();

    //create textures
    pipelineTextureProgram->Bind();
    createTexture();

    //create textures
    pipelineSkyTextureProgram->Bind();
    initSkyTexture();

    // Check for any OpenGL errors.
    std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[]) {
    cout << "Initializing GLUT..." << endl;
    glutInit(&argc, argv);

    cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(windowTitle);

    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

    // Tells GLUT to use a particular display function to redraw.
    glutDisplayFunc(displayFunc);
    // Perform animation inside idleFunc.
    //glutIdleFunc(idleFunc);
    glutTimerFunc(waitBeforeMove, timerFunc, splineIterater);
    // callback for mouse drags
    glutMotionFunc(mouseMotionDragFunc);
    // callback for idle mouse movement
    glutPassiveMotionFunc(mouseMotionFunc);
    // callback for mouse button changes
    glutMouseFunc(mouseButtonFunc);
    // callback for resizing the window
    glutReshapeFunc(reshapeFunc);
    // callback for pressing the keys on the keyboard
    glutKeyboardFunc(keyboardFunc);

    // init glew
#ifdef __APPLE__
    // nothing is needed on Apple
#else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
#endif

    // Perform the initialization.
    initScene(argc, argv);

    // Sink forever into the GLUT loop.
    glutMainLoop();
}