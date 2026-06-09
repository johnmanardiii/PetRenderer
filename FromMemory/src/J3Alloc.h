//
// Created by johnm on 5/31/2026.
//

#ifndef J3ALLOC_H
#define J3ALLOC_H

#include <vector>
#include <volk.h>

struct J3MemoryHeap
{
    bool bIsDeviceLocal = false;
    uint64_t size;
    // need to check memory type flags against these:
    // https://docs.vulkan.org/spec/latest/chapters/memory.html#vkGetPhysicalDeviceMemoryProperties2:~:text=typedef%20enum%20VkMemoryPropertyFlagBits%20%7B%0A%20%20%20%20VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT%20%3D%200x00000001%2C%0A%20%20%20%20VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT%20%3D%200x00000002%2C
    std::vector<VkMemoryType> memoryTypes;
};

/*
 * I have decided, for educational purposes, to initially avoid using VMA so that I can learn what is going on under
 * the hood with the memory allocator GPU side and the different types of memory that are available.
 *
 * J3Alloc - John Francis Manard III Memory Allocator (for device memory)
 *
 * Maybe one day I'll write one for host memory, but I have no need for that right now.
 */
class J3Alloc
{
  public:

    void Initialize(VkPhysicalDevice physDevice, VkDevice device);
    uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);

  private:
    void VerifyImageSupport();
    void ReadInProperties();
    // initializes the j3 memory heaps
    void CreateMemoryHeaps(const VkPhysicalDeviceMemoryProperties &properties);


    std::vector<J3MemoryHeap> memoryHeaps;
    // all memory types by index since the code cares about this and not necessarily the heaps.
    std::vector<VkMemoryType> memoryTypes;
    VkPhysicalDevice physDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
};

#endif // J3ALLOC_H
