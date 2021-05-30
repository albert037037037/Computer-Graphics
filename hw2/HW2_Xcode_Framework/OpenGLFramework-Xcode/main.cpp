#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 666;
const int WINDOW_HEIGHT = 500;

double lastx, lasty;
bool mousePress = false;
bool isDrawWireframe = false;
bool mouse_pressed = false;
double starting_press_x = -1;
double starting_press_y = -1;
enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
    Lighting = 6,
    Shiness = 7,
    SwitchLight = 8,
};

GLint iLocMVP;
GLint iLocMV;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;

};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
	vector<Shape> shapes;
};
vector<model> models;

// Edit hw2
struct iLoclightInfo{
    GLuint position;
    GLuint spotDir;
    GLuint spotExponent;
    GLuint spotCutoff;
    GLuint diffuse;
    GLuint Ambient;
    GLuint constantAttenuation;
    GLuint linearAttenuation;
    GLuint quadraticAttenuation;
    // Not sure
    GLuint specular;
} iLoclightInfo[3];

struct LightInfo{
    Vector3 position;
    Vector3 spotDir;
    float spotExponent;
    float spotCutoff;
    Vector3 diffuse;
    Vector3 Ambient;
    float shiness;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    // Not sure
    Vector3 specular;
} lightInfo[3];

struct iLocPhongMaterial
{
    GLuint Ka;
    GLuint Kd;
    GLuint Ks;

} iLocPhongMaterial;

int lightIndex;
int is_perpixel;
float shiness;
GLuint iLoclightIndex;
GLuint iLocviewMatrix;
GLuint iLocshiness;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;
    
    mat = Matrix4(
        1,     0,     0,     vec.x,
        0,     1,     0,     vec.y,
        0,     0,     1,     vec.z,
        0,     0,     0,      1
                  );
    return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
    Matrix4 mat;

    
    mat = Matrix4(
        vec.x, 0,  0,     0,
        0,  vec.y, 0,     0,
        0,  0,     vec.z, 0,
        0,  0,     0,     1
        );

    return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
    Matrix4 mat;

    
    mat = Matrix4(
        1, 0, 0, 0,
        0, cos(val), -sin(val), 0,
        0, sin(val), cos(val), 0,
        0, 0, 0, 1
    );
    

    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
    Matrix4 mat;

    
    mat = Matrix4(
        cos(val), 0, sin(val), 0,
        0, 1, 0, 0,
        -sin(val), 0, cos(val), 0,
        0, 0, 0, 1
    );

    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
    Matrix4 mat;

    mat = Matrix4(
        cos(val), -sin(val), 0, 0,
        sin(val), cos(val), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    return mat;
}

Matrix4 rotate(Vector3 vec)
{
    return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}


// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
    Matrix4 T = Matrix4(
    1, 0, 0, -main_camera.position.x,
    0, 1, 0, -main_camera.position.y,
    0, 0, 1, -main_camera.position.z,
    0, 0, 0, 1
                        );
    GLfloat Rx[3];
    GLfloat p1p2[3], up[3], tmp[3];
    Vector3 P1P2 = main_camera.center - main_camera.position;
    p1p2[0] = P1P2.x; p1p2[1] = P1P2.y; p1p2[2] = P1P2.z;
    tmp[0] = p1p2[0]; tmp[1] = p1p2[1]; tmp[2] = p1p2[2];
    Normalize(tmp);
    Vector3 up_v = main_camera.up_vector;
    up[0] = up_v.x; up[1] = up_v.y; up[2] = up_v.z;
    Normalize(up);
    Cross(tmp, up, Rx);
    Cross(Rx, p1p2, up);
    Normalize(Rx);
    Normalize(up);
    
    Matrix4 R = Matrix4 (
    Rx[0], Rx[1], Rx[2], 0,
    up[0], up[1], up[2], 0,
    -tmp[0], -tmp[1], -tmp[2], 0,
    0, 0, 0, 1
                         );
    
    view_matrix = R * T;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
    cur_proj_mode = Orthogonal;
    project_matrix = Matrix4(
        2/(proj.right - proj.left), 0, 0, -((proj.right + proj.left)/(proj.right - proj.left)),
        0, 2/(proj.top - proj.bottom), 0, -((proj.top + proj.bottom)/(proj.top - proj.bottom)),
        0, 0, -2 / (proj.farClip - proj.nearClip), -((proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip)),
        0, 0, 0, 1
    );
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
    cur_proj_mode = Perspective;
    
    GLfloat f = -cos(proj.fovy/2)/sin(proj.fovy/2);
    project_matrix = Matrix4(
    f/proj.aspect, 0, 0, 0,
    0, f, 0, 0,
     0, 0, (proj.farClip + proj.nearClip)/(proj.nearClip-proj.farClip), (2*proj.farClip*proj.nearClip/(proj.nearClip-proj.farClip)),
    0, 0, -1, 0);
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    proj.aspect = (float)width / (float) height;
    // cout << "In change size" << endl;
    //if(height == 0) height = 1;
    // [TODO] change your aspect ratio in both perspective and orthogonal view
    
    //glViewport(width/2-450, height/2-450, 900, 900);
}

void updateLight() {
    glUniform1i(iLoclightIndex, lightIndex);
    glUniform1f(iLocshiness, shiness);
    // three different light
    glUniform3f(iLoclightInfo[0].position, lightInfo[0].position.x, lightInfo[0].position.y, lightInfo[0].position.z);
    glUniform3f(iLoclightInfo[0].diffuse, lightInfo[0].diffuse.x, lightInfo[0].diffuse.y, lightInfo[0].diffuse.z);
    glUniform3f(iLoclightInfo[0].Ambient, lightInfo[0].Ambient.x, lightInfo[0].Ambient.y, lightInfo[0].Ambient.z);
    glUniform3f(iLoclightInfo[0].specular, lightInfo[0].specular.x, lightInfo[0].specular.y, lightInfo[0].specular.z);
    
    
    glUniform3f(iLoclightInfo[1].position, lightInfo[1].position.x, lightInfo[1].position.y, lightInfo[1].position.z);
    glUniform3f(iLoclightInfo[1].diffuse, lightInfo[1].diffuse.x, lightInfo[1].diffuse.y, lightInfo[1].diffuse.z);
    glUniform3f(iLoclightInfo[1].Ambient, lightInfo[1].Ambient.x, lightInfo[1].Ambient.y, lightInfo[1].Ambient.z);
    glUniform3f(iLoclightInfo[1].specular, lightInfo[1].specular.x, lightInfo[1].specular.y, lightInfo[1].specular.z);
    glUniform1f(iLoclightInfo[1].constantAttenuation, lightInfo[1].constantAttenuation);
    glUniform1f(iLoclightInfo[1].linearAttenuation, lightInfo[1].linearAttenuation);
    glUniform1f(iLoclightInfo[1].quadraticAttenuation, lightInfo[1].quadraticAttenuation);
    
    
    glUniform3f(iLoclightInfo[2].position, lightInfo[2].position.x, lightInfo[2].position.y, lightInfo[2].position.z);
    glUniform3f(iLoclightInfo[2].diffuse, lightInfo[2].diffuse.x, lightInfo[2].diffuse.y, lightInfo[2].diffuse.z);
    glUniform3f(iLoclightInfo[2].Ambient, lightInfo[2].Ambient.x, lightInfo[2].Ambient.y, lightInfo[2].Ambient.z);
    glUniform3f(iLoclightInfo[2].specular, lightInfo[2].specular.x, lightInfo[2].specular.y, lightInfo[2].specular.z);
    glUniform1f(iLoclightInfo[2].constantAttenuation, lightInfo[2].constantAttenuation);
    glUniform1f(iLoclightInfo[2].linearAttenuation, lightInfo[2].linearAttenuation);
    glUniform1f(iLoclightInfo[2].quadraticAttenuation, lightInfo[2].quadraticAttenuation);
    glUniform3f(iLoclightInfo[2].spotDir, lightInfo[2].spotDir.x, lightInfo[2].spotDir.y, lightInfo[2].spotDir.z);
    glUniform1f(iLoclightInfo[2].spotCutoff, lightInfo[2].spotCutoff);
    glUniform1f(iLoclightInfo[2].spotExponent, lightInfo[2].spotExponent);

}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling

    Matrix4 MVP;
    Matrix4 MV;
    T = translate(models[cur_idx].position);
    S = scaling(models[cur_idx].scale);
    R = rotate(models[cur_idx].rotation);
    MVP = project_matrix * view_matrix * T * R * S;
    MV = view_matrix * T * R * S;
	GLfloat mvp[16];
    GLfloat mv[16];

	// [TODO] multiply all the matrix
	// [TODO] row-major ---> column-major

    mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
    mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
    mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
    mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
    
    mv[0] = MV[0];  mv[4] = MV[1];   mv[8] = MV[2];    mv[12] = MV[3];
    mv[1] = MV[4];  mv[5] = MV[5];   mv[9] = MV[6];    mv[13] = MV[7];
    mv[2] = MV[8];  mv[6] = MV[9];   mv[10] = MV[10];   mv[14] = MV[11];
    mv[3] = MV[12]; mv[7] = MV[13];  mv[11] = MV[14];   mv[15] = MV[15];

	// use uniform to send mvp to vertex shader
    
    // HW2 two model in one window
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
    glUniformMatrix4fv(iLocMV, 1, GL_FALSE, mv);
    updateLight();
    // Per-vertex
    glUniform1i(is_perpixel, 0);
    glViewport(0, 0, WINDOW_WIDTH*2, WINDOW_HEIGHT*2);
    glUniformMatrix4fv(iLocviewMatrix, 1, GL_FALSE, view_matrix.getTranspose());
    
    
	for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
	{
        glUniform3f(iLocPhongMaterial.Ka, models[cur_idx].shapes[i].material.Ka.x, models[cur_idx].shapes[i].material.Ka.y, models[cur_idx].shapes[i].material.Ka.z);
        glUniform3f(iLocPhongMaterial.Kd, models[cur_idx].shapes[i].material.Kd.x, models[cur_idx].shapes[i].material.Kd.y, models[cur_idx].shapes[i].material.Kd.z);
        glUniform3f(iLocPhongMaterial.Ks, models[cur_idx].shapes[i].material.Ks.x, models[cur_idx].shapes[i].material.Ks.y, models[cur_idx].shapes[i].material.Ks.z);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}
    
    glUniform1i(is_perpixel, 1);
    glViewport(WINDOW_WIDTH*2, 0, WINDOW_WIDTH*2, WINDOW_HEIGHT*2);
    
    for (int i = 0; i < models[cur_idx].shapes.size(); i++)
    {
        glBindVertexArray(models[cur_idx].shapes[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
    }
}


void information() {
//    cout << "Translation Matrix of current model\n" << translate(models[cur_idx].position) << endl;
//    cout << "Rotation Matrix of current model\n" << rotate(models[cur_idx].rotation) << endl;
//    cout << "Scaling Matrix of current model\n" << scaling(models[cur_idx].scale) << endl;
//    cout << "Viewing Matrix\n" << view_matrix << endl;
//    cout << "Projection Matrix\n" << project_matrix;
//    cout << "------------------------------------------------\n\n";
}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // [TODO] Call back function for keyboard
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        cur_idx = (cur_idx == 0) ? 4 : cur_idx - 1;
    }
    
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        cur_idx = (cur_idx == 4) ? 0 : cur_idx + 1;
    }
    
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        isDrawWireframe = !isDrawWireframe;
    }
    
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        cur_trans_mode = GeoTranslation;
    }
    
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        cur_trans_mode = GeoScaling;
    }
    
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        cur_trans_mode = GeoRotation;
    }
    
    if(key == GLFW_KEY_I && action == GLFW_PRESS) {
        information();
    }
    if(key == GLFW_KEY_L && action == GLFW_PRESS) {
        cur_trans_mode = SwitchLight;
        lightIndex = (lightIndex == 2) ? 0 : lightIndex + 1;
        if(lightIndex == 0) cout << "Light source is directional light\n\n";
        else if(lightIndex == 1) cout << "Light source is position light\n\n";
        else if(lightIndex == 2) cout << "Light source is spot light\n\n";
    }
    if(key == GLFW_KEY_K && action == GLFW_PRESS) {
        cur_trans_mode = Lighting;
    }
    if(key == GLFW_KEY_J && action == GLFW_PRESS) {
        cur_trans_mode = Shiness;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // [TODO] scroll up positive, otherwise it would be negtive
    switch (cur_trans_mode) {
        case GeoTranslation:
            models[cur_idx].position.z -= yoffset/5;
            break;
        case GeoScaling:
            models[cur_idx].scale.z += yoffset/5;
            break;
        case GeoRotation:
            models[cur_idx].rotation.z += yoffset/5;
            break;
        case Lighting:
            if(lightIndex == 0) {
                lightInfo[lightIndex].diffuse -= Vector3(0.05, 0.05, 0.05) * yoffset;
                if(lightInfo[lightIndex].diffuse.y < 0) {
                    lightInfo[lightIndex].diffuse = Vector3(0.0, 0.0, 0.0);
                }
                else if(lightInfo[lightIndex].diffuse.y > 1) {
                    lightInfo[lightIndex].diffuse = Vector3(1.0, 1.0, 1.0);
                }
            }
            else if(lightIndex == 1) {
                lightInfo[lightIndex].diffuse -= Vector3(0.05, 0.05, 0.05) * yoffset;
                if(lightInfo[lightIndex].diffuse.z < 0) {
                    lightInfo[lightIndex].diffuse = Vector3(0.0, 0.0, 0.0);
                }
                else if(lightInfo[lightIndex].diffuse.z > 1) {
                    lightInfo[lightIndex].diffuse = Vector3(1.0, 1.0, 1.0);
                }
            }
            else {
                lightInfo[lightIndex].spotCutoff -= yoffset;
                if(lightInfo[lightIndex].spotCutoff < 0.0) {
                    lightInfo[lightIndex].spotCutoff = 0.0;
                }
                else if(lightInfo[lightIndex].spotCutoff > 90.0) {
                    lightInfo[lightIndex].spotCutoff = 90.0;
                }
            }
            break;
        case Shiness:
            shiness += yoffset;
            if(shiness > 450) {
                shiness = 450;
            }
            else if(shiness < 0){
                shiness = 0;
            }
            break;
    }
    if(cur_trans_mode == ViewCenter || cur_trans_mode == ViewEye || cur_trans_mode == ViewUp)
        setViewingMatrix();
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // [TODO] Call back function for mouse
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mousePress = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mousePress = false;
    }
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // [TODO] Call back function for cursor position
    double diffx = xpos - lastx;
    double diffy = ypos - lasty;
    
    if(mousePress) {
        switch(cur_trans_mode) {
            case GeoTranslation:
                models[cur_idx].position.x += diffx / 250.0;
                models[cur_idx].position.y -= diffy / 250.0;
                break;
            case GeoScaling:
                models[cur_idx].scale.x += (diffx / 25.0);
                models[cur_idx].scale.y -= (diffy / 25.0);
                break;
            case GeoRotation:
                models[cur_idx].rotation.y -= 3.14159265358979 / 180.0 * diffx;
                models[cur_idx].rotation.x -= 3.14159265358979 / 180.0 * diffy;
                break;
            case Lighting:
                lightInfo[lightIndex].position.x += 0.005 * diffx;
                lightInfo[lightIndex].position.y -= 0.005 * diffy;
                break;
        }
        
        if(cur_trans_mode == ViewCenter || cur_trans_mode == ViewEye || cur_trans_mode == ViewUp)
            setViewingMatrix();
    }
    
    lastx = xpos;
    lasty = ypos;
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");
    // Hw2 more parameter need to be connect
    iLocMV = glGetUniformLocation(p, "mv");
    
    is_perpixel = glGetUniformLocation(p, "is_perpixel");
    iLocviewMatrix = glGetUniformLocation(p, "viewMatrix");
    iLoclightIndex = glGetUniformLocation(p, "lightIndex");
    iLocshiness = glGetUniformLocation(p, "shiness");
    
    iLocPhongMaterial.Ka = glGetUniformLocation(p, "material.Ka");
    iLocPhongMaterial.Kd = glGetUniformLocation(p, "material.Kd");
    iLocPhongMaterial.Ks = glGetUniformLocation(p, "material.Ks");
    
    iLoclightInfo[0].position = glGetUniformLocation(p, "lightInfo[0].position");
    iLoclightInfo[0].spotDir = glGetUniformLocation(p, "lightInfo[0].spotDir");
    iLoclightInfo[0].spotExponent = glGetUniformLocation(p, "lightInfo[0].spotExponent");
    iLoclightInfo[0].spotCutoff = glGetUniformLocation(p, "lightInfo[0].spotCutoff");
    iLoclightInfo[0].diffuse = glGetUniformLocation(p, "lightInfo[0].diffuse");
    iLoclightInfo[0].Ambient = glGetUniformLocation(p, "lightInfo[0].Ambient");
    iLoclightInfo[0].constantAttenuation = glGetUniformLocation(p, "lightInfo[0].constantAttenuation");
    iLoclightInfo[0].linearAttenuation = glGetUniformLocation(p, "lightInfo[0].linearAttenuation");
    iLoclightInfo[0].quadraticAttenuation = glGetUniformLocation(p, "lightInfo[0].quadraticAttenuation");
    iLoclightInfo[0].specular = glGetUniformLocation(p, "lightInfo[0].specular");
    
    iLoclightInfo[1].position = glGetUniformLocation(p, "lightInfo[1].position");
    iLoclightInfo[1].spotDir = glGetUniformLocation(p, "lightInfo[1].spotDir");
    iLoclightInfo[1].spotExponent = glGetUniformLocation(p, "lightInfo[1].spotExponent");
    iLoclightInfo[1].spotCutoff = glGetUniformLocation(p, "lightInfo[1].spotCutoff");
    iLoclightInfo[1].diffuse = glGetUniformLocation(p, "lightInfo[1].diffuse");
    iLoclightInfo[1].Ambient = glGetUniformLocation(p, "lightInfo[1].Ambient");
    iLoclightInfo[1].constantAttenuation = glGetUniformLocation(p, "lightInfo[1].constantAttenuation");
    iLoclightInfo[1].linearAttenuation = glGetUniformLocation(p, "lightInfo[1].linearAttenuation");
    iLoclightInfo[1].quadraticAttenuation = glGetUniformLocation(p, "lightInfo[1].quadraticAttenuation");
    iLoclightInfo[1].specular = glGetUniformLocation(p, "lightInfo[1].specular");
    
    iLoclightInfo[2].position = glGetUniformLocation(p, "lightInfo[2].position");
    iLoclightInfo[2].spotDir = glGetUniformLocation(p, "lightInfo[2].spotDir");
    iLoclightInfo[2].spotExponent = glGetUniformLocation(p, "lightInfo[2].spotExponent");
    iLoclightInfo[2].spotCutoff = glGetUniformLocation(p, "lightInfo[2].spotCutoff");
    iLoclightInfo[2].diffuse = glGetUniformLocation(p, "lightInfo[2].diffuse");
    iLoclightInfo[2].Ambient = glGetUniformLocation(p, "lightInfo[2].Ambient");
    iLoclightInfo[2].constantAttenuation = glGetUniformLocation(p, "lightInfo[2].constantAttenuation");
    iLoclightInfo[2].linearAttenuation = glGetUniformLocation(p, "lightInfo[2].linearAttenuation");
    iLoclightInfo[2].quadraticAttenuation = glGetUniformLocation(p, "lightInfo[2].quadraticAttenuation");
    iLoclightInfo[2].specular = glGetUniformLocation(p, "lightInfo[2].specular");
    

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);
    
    // Hw2 new parameter initialize
    shiness = 64;
    
    lightInfo[0].position = Vector3(1.0f, 1.0f, 1.0f);
    lightInfo[0].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    lightInfo[0].Ambient = Vector3(0.15f, 0.15f, 0.15f);
    lightInfo[0].specular = Vector3(1.0f, 1.0f, 1.0f);
    
    lightInfo[1].position = Vector3(0.0f, 2.0f, 1.0f);
    lightInfo[1].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    lightInfo[1].Ambient = Vector3(0.15f, 0.15f, 0.15f);
    lightInfo[1].specular = Vector3(1.0f, 1.0f, 1.0f);
    lightInfo[1].constantAttenuation = 0.01f;
    lightInfo[1].linearAttenuation = 0.8f;
    lightInfo[1].quadraticAttenuation = 0.1f;
    
    lightInfo[2].position = Vector3(0.0f, 0.0f, 2.0f);
    lightInfo[2].spotDir = Vector3(0.0f, 0.0f, -1.0f);
    lightInfo[2].spotExponent = 50;
    lightInfo[2].spotCutoff = 30;
    lightInfo[2].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    lightInfo[2].Ambient = Vector3(0.15f, 0.15f, 0.15f);
    lightInfo[2].specular = Vector3(1.0f, 1.0f, 1.0f);
    lightInfo[2].constantAttenuation = 0.05f;
    lightInfo[2].linearAttenuation = 0.3f;
    lightInfo[2].quadraticAttenuation = 0.6f;

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
    for (int i=0; i<5; i++) {
        LoadModels(model_list[i]);
    }
    cout << "Light source is directional light\n\n";

}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH*2, WINDOW_HEIGHT, "107062321 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
