#include "camera.hpp"

Camera::Camera() {}

void Camera::set_position(float x, float y) {
    pos_x_ = x;
    pos_y_ = y;
}

void Camera::set_direction(float dir_x, float dir_y, float fov) {
    dir_x_ = dir_x;
    dir_y_ = dir_y;
    plane_x_ = -dir_y * fov;
    plane_y_ =  dir_x * fov;
}

bool Camera::project(float world_x, float world_y, float& tform_x, float& tform_y) const {
    float dx = world_x - pos_x_;
    float dy = world_y - pos_y_;

    float inv = 1.0f / (plane_x_ * dir_y_ - dir_x_ * plane_y_);
    tform_x = inv * ( dir_y_ * dx - dir_x_ * dy);
    tform_y = inv * (-plane_y_ * dx + plane_x_ * dy);

    return tform_y > 0.1f;
}
