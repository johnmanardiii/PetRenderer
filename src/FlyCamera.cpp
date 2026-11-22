//
// Created by johnm on 11/21/2026.
//

#include "FlyCamera.h"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_log.h"
#include "CustomMath.h"

#include <cassert>

void FlyCamera::HandleJoystickEvent(SDL_Event *event) {
    assert(event->type == SDL_EVENT_JOYSTICK_AXIS_MOTION);
    SDL_JoyAxisEvent &axisEvent = event->jaxis;
    // SDL_Log("Axis Event: Which %d, Axis %d, value %d\n", axisEvent.which, axisEvent.axis, axisEvent.value);
    // #JOHN - back in main (way far away) we asserted that the joystick had 4 axies, 0, 1 are for left, 2, 3 are for right (left is move, right is look)
    // #JOHN - for now (bad) ignore which joystick it is, and just process it
    // #JOHN - turn the int values into a float based on int 16 max
    // #JOHN - ok interesting so "axis motion" means "axis moved" not "poll every frame" so you want to store the values between / across frames
    float normalizedAmount = static_cast<float>(axisEvent.value) / INT16_MAX;
    if (axisEvent.axis == 0) {
        lastReportedSideInput = normalizedAmount;
    } else if (axisEvent.axis == 1) {
        lastReportedForwardInput = normalizedAmount;
    }
    else if (axisEvent.axis == 2)
    {
        lastReportedYawInput = normalizedAmount;
    }
    else if (axisEvent.axis == 3)
    {
        lastReportedPitchInput = normalizedAmount;
    }
}

Matrix4D FlyCamera::GetOrientation() const {
    return Matrix4D::MakeRotationY(yawRads) * Matrix4D::MakeRotationX(pitchRads);
}

void FlyCamera::ProcessQueuedInput(float delta) {
    Vector3D offset{lastReportedSideInput, 0.0f, -lastReportedForwardInput};
    if (MagSqrd(offset) > 0.0f) {
        if (Magnitude(offset) > 1.0f)
        {
            offset = Normalize(offset);
        }

        offset *= delta * movementSpeed;
        Matrix4D coordinateFrame = GetOrientation();
        position += (offset.x * coordinateFrame[0]) + (offset.z * coordinateFrame[2]);
    }

    // cap the pitch to always be abs value less than 90 to prevent funny business
    Vector3D rotationAmount = {lastReportedPitchInput, lastReportedYawInput, 0.0f};
    if (MagSqrd(rotationAmount) > 0.0f)
    {
        rotationAmount *= delta * rotationSpeed;
        pitchRads -= rotationAmount.x;
        yawRads += rotationAmount.y;

        // limit pitch to avoid gimbal lock n flipping upside down and stuff
        float maxPitchAbsValue = deg2rad(89);
        pitchRads = Clamp(pitchRads, -maxPitchAbsValue, maxPitchAbsValue);
    }
}

void FlyCamera::Update(float delta) {
    ProcessQueuedInput(delta);
}

Matrix4D FlyCamera::GetViewMatrix() const {
    // #JOHN - could make this more optimized by calculating it all out but don't really care for now, pretty cheap in the scheme of htings
    Matrix4D inversePitch = Matrix4D::MakeRotationX(-pitchRads);
    Matrix4D inverseYaw = Matrix4D::MakeRotationY(-yawRads);
    return GetOrientation().Transpose() * Matrix4D::Translate(-position);
}