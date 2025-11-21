#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "src/shaderloader.h"
#include "src/shapes/Cone.h"
#include "src/shapes/Cube.h"
#include "src/shapes/Cylinder.h"
#include "src/shapes/Sphere.h"

#include <glm/gtc/matrix_transform.hpp>


// ================== Rendering the Scene!

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // If you must use this function, do not edit anything above this

}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here

    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);

    glDeleteProgram(m_shader);
    this->doneCurrent();

}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    // Allows OpenGL to draw objects appropriately on top of one another
    glEnable(GL_DEPTH_TEST);
    // Tells OpenGL to only draw the front face
    glEnable(GL_CULL_FACE);
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here
    // Set clear color to black
    glClearColor(0,0,0,1);

    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/default.vert",
                                                 ":/resources/shaders/default.frag");

    m_glReady = true;

}

void Realtime::paintGL() {
    //if (m_renderData.shapes.empty()) return;
    // Clear screen color and depth before painting
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Bind Sphere Vertex Data
    glBindVertexArray(m_vao);

    // Task 2: activate the shader program by calling glUseProgram with `m_shader`
    glUseProgram(m_shader);

    // Task 6: pass in m_model as a uniform into the shader program

    // Task 7: pass in m_view and m_proj

    //test



    GLint viewLocation = glGetUniformLocation(m_shader, "view_mat");
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &m_view[0][0]);
    GLint projLocation = glGetUniformLocation(m_shader, "proj_mat");
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &m_proj[0][0]);

    //global ka kd ks

    // Task 12: pass m_ka into the fragment shader as a uniform
    GLint kaLocation = glGetUniformLocation(m_shader, "ka");
    glUniform1f(kaLocation, m_renderData.globalData.ka);

    // Task 13: pass light position and m_kd into the fragment shader as a uniform
    GLint kdLocation = glGetUniformLocation(m_shader, "kd");
    glUniform1f(kdLocation, m_renderData.globalData.kd);

    //create a struct for 8 lights

    make_light_uniforms();

    GLint camLocation = glGetUniformLocation(m_shader, "cam_pos");
    glm::vec4 cam_position = m_renderData.cameraData.pos;
    glUniform4fv(camLocation, 1, &cam_position[0]);


    // Task 14: pass shininess, m_ks, and world-space camera position
    GLint ksLocation = glGetUniformLocation(m_shader, "ks");
    glUniform1f(ksLocation, m_renderData.globalData.ks);


    int index =0;
    int offset=0;
    for (const RenderShapeData &shape : m_renderData.shapes){
        m_model = shape.ctm;
        m_normal_mat = glm::inverse(glm::transpose(glm::mat3(m_model)));

        GLint modelLocation = glGetUniformLocation(m_shader, "model_mat");
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &m_model[0][0]);

        GLint normMatLocation = glGetUniformLocation(m_shader,"normal_mat");
        glUniformMatrix3fv(normMatLocation, 1, GL_FALSE, &m_normal_mat[0][0]);

        GLint cAmbLocation = glGetUniformLocation(m_shader, "cAmbient");
        glUniform3fv(cAmbLocation, 1, &shape.primitive.material.cAmbient[0]);

        GLint cDifLocation = glGetUniformLocation(m_shader, "cDiffuse");
        glUniform3fv(cDifLocation, 1, &shape.primitive.material.cDiffuse[0]);

        GLint cSpeLocation = glGetUniformLocation(m_shader, "cSpecular");
        glUniform3fv(cSpeLocation, 1, &shape.primitive.material.cSpecular[0]);

        GLint nLocation = glGetUniformLocation(m_shader, "n");
        glUniform1f(nLocation, shape.primitive.material.shininess);

        // Draw Command
        int verts = m_sizes_v_m_shapesData[index] / 6;
        glDrawArrays(GL_TRIANGLES, offset, verts);
        index++;
        offset+= verts;
    }

    // Unbind Vertex Array
    glBindVertexArray(0);

    // Task 3: deactivate the shader program by passing 0 into glUseProgram
    glUseProgram(0);

}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    //glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    glViewport(0, 0, w * m_devicePixelRatio, h * m_devicePixelRatio);
    // Students: anything requiring OpenGL calls when the program starts should be done here
    //if (m_glReady){
        rebuildGeometry();
        updateProjMat();
        updateViewMat();
    //} //because uses width and height for the aspect ratio
}

void Realtime::sceneChanged() {
    std::cout<<m_lights4GPU.size()<<std::endl;
    m_lights4GPU.clear();
    SceneParser::parse(settings.sceneFilePath, m_renderData);
    //if (m_glReady){
        rebuildGeometry();
        updateProjMat();
        updateViewMat();
    //}

    // Ask Qt to repaint the OpenGL widget
    update();
}


void Realtime::settingsChanged() {
    std::cout<<m_lights4GPU.size()<<std::endl;
    m_lights4GPU.clear();

    //if (m_glReady){
        rebuildGeometry();
        std::cout<<"after rebuild" <<m_lights4GPU.size()<<std::endl;
        updateProjMat();
        updateViewMat();

    //}
    update(); // asks for a PaintGL() call to occur
}

void Realtime::rebuildGeometry() {
    if (!m_glReady) return;

    m_shapesData.clear();
    m_sizes_v_m_shapesData.clear();
    parse_lights(m_renderData.lights);

    // Re-generate vertex data for every shape
    for (auto &shape : m_renderData.shapes) {
        std::vector<float> data =
            generateShapeData(shape,
                              settings.shapeParameter1,
                              settings.shapeParameter2);

        m_sizes_v_m_shapesData.push_back(data.size());
        m_shapesData.insert(m_shapesData.end(), data.begin(), data.end());
    }

    // Upload to GPU
    if (m_vao == 0) {
        glGenVertexArrays(1, &m_vao);
    }
    std::cout<<"vao "<< m_vao<<std::endl;
    std::cout<<"vbo "<< m_vbo<<std::endl;
    glBindVertexArray(m_vao);

    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(GL_ARRAY_BUFFER,
                 m_shapesData.size() * sizeof(GLfloat),
                 m_shapesData.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


// ================== Camera Movement!

void Realtime::keyPressEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = true;
        m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseDown) {
        int posX = event->position().x();
        int posY = event->position().y();
        int deltaX = posX - m_prev_mouse_pos.x;
        int deltaY = posY - m_prev_mouse_pos.y;
        m_prev_mouse_pos = glm::vec2(posX, posY);

        // Use deltaX and deltaY here to rotate

        float sens =0.01f;
        glm::vec3 look_n = glm::normalize(m_renderData.cameraData.look);
        glm::vec3 up_n = glm::normalize(m_renderData.cameraData.up);

        rotateCamera(deltaX*sens, glm::vec3(0.f,1.f,0.f));
        glm::vec3 axis = glm::cross(look_n,up_n);
        rotateCamera(deltaY*sens, axis);

        update(); // asks for a PaintGL() call to occur
    }
}

void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    // Use deltaTime and m_keyMap here to move around
    glm::vec3 look_n = glm::normalize(m_renderData.cameraData.look);
    glm::vec3 up_n = glm::normalize(m_renderData.cameraData.up);

    if (m_keyMap[Qt::Key_W]) {
        translateCamera( look_n* 5.f *deltaTime );
    }
    if (m_keyMap[Qt::Key_S]) {
        translateCamera(-look_n * 5.f *deltaTime );
    }
    if (m_keyMap[Qt::Key_A]) {
        glm::vec4 dir = glm::vec4(glm::cross(glm::vec3(look_n),glm::vec3(up_n)),1.0f);
        translateCamera( dir * 5.f *deltaTime );
    }
    if (m_keyMap[Qt::Key_D]) {
        glm::vec4 dir = glm::vec4(glm::cross(glm::vec3(look_n),glm::vec3(up_n)),1.0f);
        translateCamera( -dir * 5.f *deltaTime );
    }
    if (m_keyMap[Qt::Key_Space]) {
        glm::vec4 dir = glm::vec4(0.f, 1.f,0.f,1.0f);
        translateCamera( dir * 5.f *deltaTime );
    }
    if (m_keyMap[Qt::Key_Control]) {
        glm::vec4 dir = glm::vec4(0.f, 1.f,0.f,1.0f);
        translateCamera( -dir * 5.f *deltaTime );
    }

    update(); // asks for a PaintGL() call to occur
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}

void Realtime::updateProjMat(){
    float n = settings.nearPlane;
    float f = settings.farPlane;
    float c = -n/f;

    float thetaH = m_renderData.cameraData.heightAngle;
    float aspectRatio = (float)width() /(float)height();


    glm::mat4 gl_convention(1.f,0.f,0.f,0.f,
                              0.f,1.f,0.f,0.f,
                              0.f,0.f,-2.f,0.f,
                              0.f,0.f,-1.f,1.f);

    glm::mat4 unhinge(
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f/(1.f+c), -1.f,
        0.f, 0.f, -c/(1.f+c), 0.f
        );

    glm::mat4 scaling_perspective(
        1.f / (f*aspectRatio * tan(thetaH * 0.5f)), 0, 0, 0,
        0, 1.f / (f*tan(thetaH * 0.5f)), 0, 0,
        0, 0, 1.f/f, 0,
        0, 0, 0, 1.f
        );


    m_proj = gl_convention*unhinge*scaling_perspective;

}

void Realtime::translateCamera(glm::vec3 dir){


    glm::mat4 M_trans = glm::mat4(1.f,0.f,0.f,0.f,
                                  0.f,1.f,0.f,0.f,
                                  0.f,0.f,1.f,0.f,
                                  dir.x,dir.y,dir.z,1.f);
    m_renderData.cameraData.pos = M_trans * m_renderData.cameraData.pos;
    m_renderData.cameraData.look = M_trans * m_renderData.cameraData.look;
    m_renderData.cameraData.up = M_trans * m_renderData.cameraData.up;
    updateViewMat();
}

void Realtime::rotateCamera(float angleRad, glm::vec3 axis) {
    axis = glm::normalize(axis);
    float c = cos(angleRad);
    float s = sin(angleRad);
    float t = 1 - c;

    glm::mat4 M_rot(
        t*axis.x*axis.x + c,         t*axis.x*axis.y - s*axis.z,   t*axis.x*axis.z + s*axis.y,  0,
        t*axis.x*axis.y + s*axis.z,  t*axis.y*axis.y + c,          t*axis.y*axis.z - s*axis.x,  0,
        t*axis.x*axis.z - s*axis.y,  t*axis.y*axis.z + s*axis.x,   t*axis.z*axis.z + c,         0,
        0,                           0,                            0,                           1
        );

    m_renderData.cameraData.look = M_rot * m_renderData.cameraData.look;
    m_renderData.cameraData.up   = M_rot * m_renderData.cameraData.up;

    updateViewMat();
}


void Realtime::updateViewMat(){
    SceneCameraData camData = m_renderData.cameraData;
    m_camera = Camera(camData.pos, camData.look, camData.up);
    m_view = m_camera.viewMat;

}

std::vector<float> Realtime::generateShapeData(RenderShapeData& shape_to_add,
                                                   GLuint param1, GLuint param2)
    {
        std::unique_ptr<Shape> shape;

        switch (shape_to_add.primitive.type) {
        case PrimitiveType::PRIMITIVE_CUBE:
            shape = std::make_unique<Cube>();
            break;
        case PrimitiveType::PRIMITIVE_CONE:
            shape = std::make_unique<Cone>();
            break;
        case PrimitiveType::PRIMITIVE_CYLINDER:
            shape = std::make_unique<Cylinder>();
            break;
        case PrimitiveType::PRIMITIVE_SPHERE:
            shape = std::make_unique<Sphere>();
            break;
        default:
            return {};
        }
        shape->setVertexData(param1, param2);

        return shape->m_vertexData;
    }

void Realtime::parse_lights (std::vector<SceneLightData>& lights){
    //empty the previous lights
    m_lights4GPU.clear();

    int i =0;
    for (SceneLightData light : lights){
        m_lights4GPU.push_back(Light4GPU());

        m_lights4GPU[i].valid = 1;
        m_lights4GPU[i].type = int(light.type);
        m_lights4GPU[i].color = light.color;
        m_lights4GPU[i].function = light.function;
        switch (light.type){
        case(LightType::LIGHT_POINT):
            m_lights4GPU[i].pos = light.pos;
            break;
        case(LightType::LIGHT_DIRECTIONAL):
            m_lights4GPU[i].dir = light.dir;
            break;
        case(LightType::LIGHT_SPOT):
            m_lights4GPU[i].pos = light.pos;
            m_lights4GPU[i].dir = light.dir;
            m_lights4GPU[i].penumbra = light.penumbra;
            m_lights4GPU[i].angle = light.angle;
            break;
        }
        i++;
    }
}

void Realtime::make_light_uniforms(){

    for (int j = 0; j < 8; j++) {
        auto sendInt = [&](std::string name, int value){
            GLint loc = glGetUniformLocation(m_shader, name.c_str());
            glUniform1i(loc, value);
        };

        auto sendFloat = [&](std::string name, float value){
            GLint loc = glGetUniformLocation(m_shader, name.c_str());
            glUniform1f(loc, value);
        };

        auto sendVec3 = [&](std::string name, const glm::vec3 &v){
            GLint loc = glGetUniformLocation(m_shader, name.c_str());
            glUniform3f(loc, v.x, v.y, v.z);
        };

        if (j>=m_lights4GPU.size()){
            sendInt("lights_8[" + std::to_string(j) + "].valid", 0);
            continue;
        }


        const Light4GPU &L = m_lights4GPU[j];

        sendInt("lights_8[" + std::to_string(j) + "].valid", L.valid);
        sendInt("lights_8[" + std::to_string(j) + "].type", L.type);
        sendVec3("lights_8[" + std::to_string(j) + "].color", L.color);
        sendVec3("lights_8[" + std::to_string(j) + "].function", L.function);
        sendVec3("lights_8[" + std::to_string(j) + "].pos", L.pos);
        sendVec3("lights_8[" + std::to_string(j) + "].dir", L.dir);
        sendFloat("lights_8[" + std::to_string(j) + "].penumbra", L.penumbra);
        sendFloat("lights_8[" + std::to_string(j) + "].angle", L.angle);
    }
}
