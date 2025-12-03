#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings {
    std::string sceneFilePath;
    int shapeParameter1 = 1;
    int shapeParameter2 = 1;
    float nearPlane = 1;
    float farPlane = 1;
    bool perPixelFilter = false;
    bool kernelBasedFilter = false;
    bool extraCredit1 = false;
    bool extraCredit2 = false;
    bool extraCredit3 = false;
    bool extraCredit4 = false;
    bool show_depth = false; //has replaced extra credit 1!!!
    bool show_velocity_x =false; //has replaced extra credit 2!!!
    bool show_velocity_y =false; //has replaced extra credit 3!!!
    bool enable_motion_blur = false; //has replaced extra credit 4!!!
};


// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H
