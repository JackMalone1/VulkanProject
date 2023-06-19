#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

class Mesh
{
public:
    Mesh();
    Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices);
    int getVertexCount();
    VkBuffer getVertexBuffer();
    void destroyVertexBuffer();
private:
    int vertexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    void create_vertex_buffer(std::vector<Vertex>* vertices);
    uint32_t findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties);
};
