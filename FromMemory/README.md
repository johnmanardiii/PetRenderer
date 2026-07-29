# Writing Vulkan code from memory

* this project is me trying to use only the spec to write a vulkan program after reading vkguide. no AI, no tutorial
  code.
* The sole purpose of this project is learning.

# Dependencies:
If running on Linux, please install the depedencies for SDL3: https://wiki.libsdl.org/SDL3/README-linux

Ensure that the Vulkan SDK is installed on your computer: https://vulkan.lunarg.com/
(if on Linux I found this useful: https://vulkan.lunarg.com/doc/sdk/1.4.357.0/linux/getting_started.html, make sure you follow the 
steps to add the needed things to your env on linux, I had to manually add the env
var to the cmake config since clion wasn't picking up on it.)

## 4/26/2026

* decided to go with creating my swapchain images in sRGB since that made the most sense to me for what would be least
  confusing in this project,
  which is mostly just trying to create my own personal sandbox renderer with imgui and such and not some crazy
  application that exists
  within a larger ecosystem.

## 5/2/2026

* decided to make the swapchain something that is a destination target, and has the exclusive sharing mode

## 5/7/2026

* learned that there are transforms on the swapchain that determine the "transform" that mainly handle mobile device
  rotation. just going to verify current transform is valid and pass that forward. if devices ever rotate (or the
  swapchain code is super inflexible) come back to this.
* for now just set the composite alpha to VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR since this is for rendering and we don't
  need it to be anything else for now
* I decided to go with clipping on the swapchain
* should probably read this if my device ever
  rotates: https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html. decided not
  to
* read this if pre transform needs to be handled. for now, if not identity,
  assert: https://github.com/google/vulkan-pre-rotation-demo

## 5/12/2026

* going to make one command pool / buffer per swapchain image, just for drawing. wish me luck

## 5/14/2026

* decided against this since it made it more confusing and I don't care about multiple devices in this
  project: https://docs.vulkan.org/refpages/latest/refpages/source/vkAcquireNextImage2KHR.html

## 5/17/2026

* in the instance builder you quietly set the version to 1_3, but you should pass that in somewhere way higher level
  like "engine" so you can figure out that value later.
* Ok, when I get this tomorrow, I need to actually call vkGetDeviceQueue and do a little audit to make sure that I haven't accidentally used a raw int somewhere stupid instead of an actual handle that I should have gotten. Then I need to figure out that semaphore situation with the queue submit and then present. and then I need to add fences. much to do but should be easier than this image memory barrier stuff.

## 5/18/2026
* when you get back to this, make sure the per frame things are cleaned up, the semaphores per swapchain image are cleaned up, then figure out where you need the CPU to wait, and also just in general get your mind around the synchronization stuff.


## 5/23/2026
* coming back to this project, the two biggest things that are bothering me are:
  * the whole "clear" swapchain and write color (technically transfer color) is all within the engine. it should be structured as: "hey report back to me when you have something for the swapchain"
  * my swapchain doesn't rebuild itself ever... so it will be begging for release and i am currently ignoring it... it eats at my conscious 