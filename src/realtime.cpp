#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "src/shaderloader.h"

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

    glDeleteVertexArrays(1, &m_fullscreen_vao);
    glDeleteBuffers(1, &m_fullscreen_vbo);

    glDeleteProgram(m_shader);
    glDeleteProgram(m_texture_shader);
    glDeleteProgram(m_depth_shader);
    glDeleteProgram(m_velocity_shader);
    glDeleteProgram(m_motion_blur_shader);

    glDeleteTextures(1, &m_fbo_scene_depth);
    glDeleteTextures(1, &m_fbo_scene_texture);
    glDeleteTextures(1, &m_fbo_depth_texture);
    glDeleteTextures(1, &m_fbo_vel_texture);

    glDeleteFramebuffers(1, &m_fbo_scene);
    glDeleteFramebuffers(1, &m_fbo_depth);
    glDeleteFramebuffers(1, &m_fbo_vel);

    this->doneCurrent();
}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();
    m_screen_width = size().width() * m_devicePixelRatio;
    m_screen_height = size().height() * m_devicePixelRatio;
    m_fbo_width = m_screen_width;
    m_fbo_height = m_screen_height;

    m_defaultFBO = 4;

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

    m_texture_shader = ShaderLoader::createShaderProgram(":/resources/shaders/texture.vert", ":/resources/shaders/texture.frag");

    m_depth_shader = ShaderLoader::createShaderProgram(":/resources/shaders/depth.vert",
                                                       ":/resources/shaders/depth.frag");

    m_velocity_shader = ShaderLoader::createShaderProgram(":/resources/shaders/velocity.vert",
                                                       ":/resources/shaders/velocity.frag");

    m_motion_blur_shader = ShaderLoader::createShaderProgram(":/resources/shaders/motion_blur.vert",
                                                             ":/resources/shaders/motion_blur.frag");

    rebuildGeometry();
    updateProjMat();
    updateViewMat();

    std::vector<GLfloat> fullscreen_quad_data =
        { //     POSITIONS    // +//UV//
            -1.f,  1.f, 0.0f,
            0.f,1.f,
            -1.f, -1.f, 0.0f,
            0.f,0.f,
            1.f, -1.f, 0.0f,
            1.f, 0.f,

            -1.f,  1.f, 0.0f,
            0.f,1.f,
            1.f,  -1.f, 0.0f,
            1.f, 0.f,
            1.f, 1.f, 0.0f,
            1.f,1.f
        };

    // Generate and bind a VBO and a VAO for a fullscreen quad
    glGenBuffers(1, &m_fullscreen_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreen_vbo);
    glBufferData(GL_ARRAY_BUFFER, fullscreen_quad_data.size()*sizeof(GLfloat), fullscreen_quad_data.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &m_fullscreen_vao);
    glBindVertexArray(m_fullscreen_vao);

    // Task 14: modify the code below to add a second attribute to the vertex attribute array
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) (0* sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) (3* sizeof(GLfloat)));


    // Unbind the fullscreen quad's VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    makeFBOs();

    m_glReady = true;
}

void Realtime::paintGL() {
    //puts depth and the regular phonged geometry in the scene fbo
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_scene);
    glViewport(0,0,m_fbo_width, m_fbo_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    paintGeometry();

    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

    if (settings.show_depth) {
        // 1. Paint depth as colors in my depth buffer texture
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_depth);
        glViewport(0,0,m_fbo_width, m_fbo_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        paintDepth();

        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

        // 2. Paint depth on the screen
        paintTexture(m_fbo_depth_texture, false);

    }
    else if (settings.show_velocity_x || settings.show_velocity_y){
        // 1. Paint in my velocity buffer as colors in a texture
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_vel);
        glViewport(0,0,m_fbo_width, m_fbo_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        paintVel(settings.show_velocity_x);

        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

        // 2. Paint depth on the screen
        paintTexture(m_fbo_vel_texture, false);
    }
    else if (settings.enable_motion_blur){
        paintBlurredScene();
    }
    else{
        paintTexture(m_fbo_scene_texture, false);
    }
}



void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    //glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    glViewport(0, 0, w * m_devicePixelRatio, h * m_devicePixelRatio);
    // Students: anything requiring OpenGL calls when the program starts should be done here

    rebuildGeometry();
    updateProjMat();
    updateViewMat();
    //because uses width and height for the aspect ratio
}

void Realtime::sceneChanged() {
    m_lights4GPU.clear();
    SceneParser::parse(settings.sceneFilePath, m_renderData);

    rebuildGeometry();
    updateProjMat();
    updateViewMat();


    // Ask Qt to repaint the OpenGL widget
    update();
}


void Realtime::settingsChanged() {
    m_lights4GPU.clear();

    rebuildGeometry();
    updateProjMat();
    updateViewMat();

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
    if (!m_mouseDown) return;

    float sens = 0.01f;

    int posX = event->position().x();
    int posY = event->position().y();
    int deltaX = posX - m_prev_mouse_pos.x;
    int deltaY = posY - m_prev_mouse_pos.y;
    m_prev_mouse_pos = glm::vec2(posX, posY);


    glm::vec4 center(0.f); //orbit arount the origin = boat position

    glm::vec4 pos = m_renderData.cameraData.pos;
    glm::vec4 rel = pos - center;

    float yaw = deltaX * sens;
    glm::vec3 axisY(0.f, 1.f, 0.f);

    float c = cos(yaw), s = sin(yaw), t = 1 - c;
    glm::vec3 a = glm::normalize(axisY);

    glm::mat4 R_yaw(
        t*a.x*a.x + c,       t*a.x*a.y - s*a.z, t*a.x*a.z + s*a.y, 0,
        t*a.x*a.y + s*a.z,   t*a.y*a.y + c,     t*a.y*a.z - s*a.x, 0,
        t*a.x*a.z - s*a.y,   t*a.y*a.z + s*a.x, t*a.z*a.z + c,     0,
        0,                   0,                 0,                 1
        );

    rel = glm::vec4(R_yaw * rel);
    rotateCamera(yaw, axisY);

    float pitch = deltaY * sens;

    glm::vec3 look = glm::normalize(center - (rel + center));
    glm::vec3 up   = glm::normalize(m_renderData.cameraData.up);
    glm::vec3 right = glm::normalize(glm::cross(look, up));

    a = glm::normalize(right);
    c = cos(pitch); s = sin(pitch); t = 1 - c;

    glm::mat4 R_pitch(
        t*a.x*a.x + c,       t*a.x*a.y - s*a.z, t*a.x*a.z + s*a.y, 0,
        t*a.x*a.y + s*a.z,   t*a.y*a.y + c,     t*a.y*a.z - s*a.x, 0,
        t*a.x*a.z - s*a.y,   t*a.y*a.z + s*a.x, t*a.z*a.z + c,     0,
        0,                   0,                 0,                 1
        );

    rel = R_pitch * rel;

    rotateCamera(pitch, right);

    m_renderData.cameraData.pos = center + rel;
    m_renderData.cameraData.look =
        glm::normalize(center - m_renderData.cameraData.pos);

    update();
}


//boat rotates around its center not center of the world
glm::mat4 rotateAroundLocalY(const glm::mat4 &m, float angle) {
    // Rotate around local Y axis
    glm::vec3 pos = glm::vec3(m[3]);       // save current position
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0));
    glm::mat4 result = rot * m;
    result[3] = glm::vec4(pos, 1.0f);      // restore position
    return result;
}

void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();


    glm::vec3 look_n = glm::normalize(m_renderData.cameraData.look);

    if (m_keyMap[Qt::Key_I])
        translateCamera(glm::vec4(look_n * 5.f * deltaTime, 0.f));
    if (m_keyMap[Qt::Key_O])
        translateCamera(glm::vec4(-look_n * 5.f * deltaTime, 0.f));


    if (m_keyMap[Qt::Key_W] || m_keyMap[Qt::Key_S]) {
        float yaw = (m_keyMap[Qt::Key_W] ? +1.f : -1.f) * deltaTime * 3.5f;
        m_boatCTM = rotateAroundLocalY(m_boatCTM, yaw);
    }

    if (m_keyMap[Qt::Key_Space]) {
        float speed = 5.f * deltaTime;

        glm::vec3 forward = -glm::normalize(glm::vec3(m_boatCTM[2]));
        glm::vec3 delta = forward * speed;

        // Translate boat
        m_boatCTM = glm::translate(glm::mat4(1.0f), delta) * m_boatCTM;

        // Move camera along with the boat
        translateCamera(glm::vec4(delta, 0.f));
    }


    for (RenderShapeData &shape : m_renderData.shapes) {
        if (!shape.is_land) {
            shape.ctm = m_boatCTM * shape.local_ctm;
        }
    }


    update(); // trigger repaint
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

