#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>

#include "utils/sceneparser.h"
#include "utils/camera.h"

struct Light4GPU
{
    int valid = 0;
    int type; //0point 1dir 2spot
    glm::vec3 color;
    glm::vec3 function;
    glm::vec3 pos; //ctm already applied
    glm::vec3 dir; //ctm already applied
    float penumbra;
    float angle;
};


class Realtime : public QOpenGLWidget
{
public:
    Realtime(QWidget *parent = nullptr);
    void finish();                                      // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height) override;      // Called when window size changes

private:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // Tick Related Variables
    int m_timer;                                        // Stores timer which attempts to run ~60 times per second
    QElapsedTimer m_elapsedTimer;                       // Stores timer which keeps track of actual time between frames

    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::vec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<Qt::Key, bool> m_keyMap;         // Stores whether keys are pressed or not

    // Device Correction Variables
    double m_devicePixelRatio;

    RenderData m_renderData;
    Camera m_camera; //has the attribute m_camera.inv_view
    glm::mat4 m_model = glm::mat4(1);
    glm::mat4 m_view  = glm::mat4(1);
    glm::mat4 m_proj  = glm::mat4(1);

    void updateProjMat();
    void updateViewMat();

    GLuint m_shader;     // Stores id of shader program
    GLuint m_vbo=0; // Stores id of vbo
    GLuint m_vao =0; // Stores id of vao

    //optimization
    glm::mat3 m_normal_mat;

    std::vector<float> generateShapeData(RenderShapeData& shape, GLuint param1, GLuint param2);
    std::vector<float> m_shapesData;
    std::vector<int> m_sizes_v_m_shapesData;
    std::vector<Light4GPU> m_lights4GPU;
    void parse_lights (std::vector<SceneLightData>& lights);
    void make_light_uniforms();

    void rebuildGeometry();
    bool m_glReady = false; //cannot rebuild geometry before initialize gl ran

    void translateCamera(glm::vec3 dir);
    void rotateCamera(float angleRad, glm::vec3 axis);

    // QPoint m_prevMousePos;
    // float  m_angleX;
    // float  m_angleY;
    // float  m_zoom;
};
