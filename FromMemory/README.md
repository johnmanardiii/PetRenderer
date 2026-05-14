# Writing Vulkan code from memory
* this project is me trying to use only the spec to write a vulkan program after reading vkguide. no AI, no tutorial code.
* The sole purpose of this project is learning.

## 4/26/2026
* decided to go with creating my swapchain images in sRGB since that made the most sense to me for what would be least confusing in this project,
which is mostly just trying to create my own personal sandbox renderer with imgui and such and not some crazy application that exists
within a larger ecosystem.

## 5/2/2026
* decided to make the swapchain something that is a destination target, and has the exclusive sharing mode

## 5/7/2026
* learned that there are transforms on the swapchain that determine the "transform" that mainly handle mobile device rotation. just going to verify current transform is valid and pass that forward. if devices ever rotate (or the swapchain code is super inflexible) come back to this.
* for now just set the composite alpha to VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR since this is for rendering and we don't need it to be anything else for now
* I decided to go with clipping on the swapchain
* should probably read this if my device ever rotates: https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html. decided not to 
* read this if pre transform needs to be handled. for now, if not identity, assert: https://github.com/google/vulkan-pre-rotation-demo

## 5/12/2026
* going to make one command pool / buffer per swapchain image, just for drawing. wish me luck