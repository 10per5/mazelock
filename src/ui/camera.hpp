#pragma once

class Camera {
public:
    Camera();

    void set_position(float x, float y);
    void set_direction(float dir_x, float dir_y, float fov);

    float pos_x() const { return pos_x_; }
    float pos_y() const { return pos_y_; }

    // Project world position to camera-space; returns false if behind camera
    bool project(float world_x, float world_y, float& tform_x, float& tform_y) const;

private:
    float pos_x_ = 0.0f, pos_y_ = 0.0f;
    float dir_x_ = 1.0f, dir_y_ = 0.0f;
    float plane_x_ = 0.0f, plane_y_ = 0.66f;
};
