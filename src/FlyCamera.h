//
// Created by johnm on 11/21/2026.
//

#ifndef FLYCAMERA_H
#define FLYCAMERA_H
#include "Matrix4D.h"
#include "Vector3D.h"

union SDL_Event;

class FlyCamera
{

public:
    // #JOHN - should really be polling something central but this works for now I think
    void HandleJoystickEvent(SDL_Event* event);
    void Update(float delta);
    Matrix4D GetViewMatrix() const;
private:
    void ProcessQueuedInput(float delta);

    Vector3D position {0.0f};

    float pitchRads = 0.0f;
    float yawRads = 0.0f;
    float rotationSpeed = 1.0f;
    float movementSpeed = 7.0f;

    // #JOHN - put the perspective matrix stuff in here, maybe at least FOV.


    // queued joystick things that get set (and cleared on update / processed)
    float lastReportedForwardInput = 0.0f;
    float lastReportedSideInput = 0.0f;
    float lastReportedYawInput = 0.0f;
    float lastReportedPitchInput = 0.0f;
};

#endif //FLYCAMERA_H
