#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdbool.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "cglm/cglm.h"
#include <unistd.h>
#include <stdlib.h>
#include "meshoptimizer/src/meshoptimizer.h"
#define  FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#define NUM_DESCRIPTOR_SETS 1
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT
#define NUM_VIEWPORTS 1
#define NUM_SCISSORS NUM_VIEWPORTS
#define FENCE_TIMEOUT 100000000

typedef struct vertex {
    vec3 pos;
    vec3 color;
    vec2 textcoords;
} vertex;

typedef struct mesh {
} mesh;

#if 0
uint32_t index_data[]  = {
    0, 3, 2,
    0, 2, 1
};
#endif
void get_file_data(const char *path, char **buffer, size_t *length) {
    FILE *file = fopen(path, "r");
    assert(file);
    
	fseek(file, 0, SEEK_END);
	*length = ftell(file);
	assert(*length >= 0);
	fseek(file, 0, SEEK_SET);
    
	*buffer = (char *)malloc(*length * sizeof(char));
	assert(buffer);
    
	size_t rc = fread(*buffer, 1, *length, file);
	//assert(rc == sizeof(length));
	fclose(file);
    
}

char *read_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "r");
    assert(file);
    
	fseek(file, 0, SEEK_END);
	*length = ftell(file);
	assert(*length >= 0);
	fseek(file, 0, SEEK_SET);
    
	char *buffer = (char *)malloc(*length * sizeof(char));
	assert(buffer);
    
	size_t rc = fread(buffer, 1, *length, file);
	//assert(rc == sizeof(length));
	fclose(file);
    
    return buffer;
}

#if 0
typedef struct Vertex
{
	float vx, vy, vz;
	uint8_t nx, ny, nz, nw;
	uint16_t tu, tv;
} Vertex;

typedef struct Mesh {
    Vertex *vertices;
    unsigned int *indices;
} Mesh;

Mesh parseObj(const char* path, double *reindex)
{
	fastObjMesh* obj = fast_obj_read(path);
    assert(obj);
	/*if (!obj)
	{
		printf("Error loading %s: file not found\n", path);
		return Mesh();
	}*/
    
	size_t total_indices = 0;
    
	for (unsigned int i = 0; i < obj->face_count; ++i) {
		total_indices += 3 * (obj->face_vertices[i] - 2);
    }
    
    Vertex vertices[total_indices];
	//std::vector<Vertex> vertices(total_indices);
    
	size_t vertex_offset = 0;
	size_t index_offset = 0;
    
	for (unsigned int i = 0; i < obj->face_count; ++i)
	{
		for (unsigned int j = 0; j < obj->face_vertices[i]; ++j)
		{
			fastObjIndex gi = obj->indices[index_offset + j];
            
			Vertex v =
            {
                obj->positions[gi.p * 3 + 0],
                obj->positions[gi.p * 3 + 1],
                obj->positions[gi.p * 3 + 2],
                obj->normals[gi.n * 3 + 0],
                obj->normals[gi.n * 3 + 1],
                obj->normals[gi.n * 3 + 2],
                obj->texcoords[gi.t * 2 + 0],
                obj->texcoords[gi.t * 2 + 1],
            };
            
			// triangulate polygon on the fly; offset-3 is always the first polygon vertex
			if (j >= 3)
			{
				vertices[vertex_offset + 0] = vertices[vertex_offset - 3];
				vertices[vertex_offset + 1] = vertices[vertex_offset - 1];
				vertex_offset += 2;
			}
            
			vertices[vertex_offset] = v;
			vertex_offset++;
		}
        
		index_offset += obj->face_vertices[i];
	}
    
	fast_obj_destroy(obj);
    
	//reindex = timestamp();
    
	Mesh result;
    /*
	std::vector<unsigned int> remap(total_indices);
    
	size_t total_vertices = meshopt_generateVertexRemap(&remap[0], NULL, total_indices, &vertices[0], total_indices, sizeof(Vertex));
    
	result.indices.resize(total_indices);
	meshopt_remapIndexBuffer(&result.indices[0], NULL, total_indices, &remap[0]);
    
	result.vertices.resize(total_vertices);
	meshopt_remapVertexBuffer(&result.vertices[0], &vertices[0], total_indices, sizeof(Vertex), &remap[0]);
    */
	return result;
}
#endif

bool get_memory_type(VkPhysicalDeviceMemoryProperties physical_device_memory_properties, uint32_t type_bits, VkMemoryPropertyFlags memory_property_flags, uint32_t *type_index) {
    for (int i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if ((type_bits & 1) == 1) {
            if ((physical_device_memory_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags) {
                *type_index = i;
                return true;
            }
        }
        type_bits >>= 1;
    }
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                              size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
                                              void *pUserData) {
    char *message;
    
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        message = "ERROR: ";
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        message = "WARNING: ";
    } else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        message = "PERFORMANCE WARNING: ";
    } else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        message = "INFO: ";
    } else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        message = "DEBUG: ";
    }
    
    printf("%s[%s] Code %d : %s\n", message, pLayerPrefix, msgCode, pMsg);
    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    
}

void create_buffer(VkBuffer *buffer, VkDeviceSize buffer_size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkDevice device, VkPhysicalDeviceMemoryProperties physical_device_memory_properties, VkDeviceMemory *device_memory) {
    
    VkResult result; 
    
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = NULL;
    buffer_create_info.usage = buffer_usage_flags;
    buffer_create_info.size = buffer_size;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = NULL;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.flags = 0;
    result = vkCreateBuffer(device, &buffer_create_info, NULL, buffer);
    assert(result == VK_SUCCESS);
    
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(device, *buffer, &buffer_memory_requirements);
    
    VkMemoryAllocateInfo buffer_memory_allocate_info = {};
    buffer_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    buffer_memory_allocate_info.pNext = NULL;
    buffer_memory_allocate_info.allocationSize = buffer_memory_requirements.size;
    get_memory_type(physical_device_memory_properties, buffer_memory_requirements.memoryTypeBits,
                    memory_property_flags,
                    &buffer_memory_allocate_info.memoryTypeIndex);
    
    //NOTE: niagara uses 0 as its device memory 
    result = vkAllocateMemory(device, &buffer_memory_allocate_info, NULL, device_memory);
    assert(result == VK_SUCCESS);
    
    result = vkBindBufferMemory(device, *buffer, *device_memory, 0);
    assert(result == VK_SUCCESS);
    
}

void copy_buffer(VkBuffer src_buffer, VkBuffer dest_buffer, VkDeviceSize size, VkCommandPool command_pool, VkDevice device, VkQueue graphics_queue) {
    
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);
    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0; 
    copy_region.dstOffset = 0; 
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dest_buffer, 1, &copy_region);
    vkEndCommandBuffer(command_buffer);
    
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);
    
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    
}

void create_image(VkImage *image, VkDeviceMemory *device_memory, uint32_t width, uint32_t height, VkFormat format, VkImageTiling image_tiling, VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkDevice device, VkPhysicalDeviceMemoryProperties physical_device_memory_properties) {
    VkResult result;
    
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = image_tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = image_usage_flags;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    result = vkCreateImage(device, &image_create_info, NULL, image);
    assert(result == VK_SUCCESS);
    
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, *image, &memory_requirements);
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    get_memory_type(physical_device_memory_properties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_allocate_info.memoryTypeIndex);
    
    result = vkAllocateMemory(device, &memory_allocate_info, NULL, device_memory);
    assert(result == VK_SUCCESS);
    
    vkBindImageMemory(device, *image, *device_memory, 0);
}

VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
    
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue graphicsQueue, VkDevice device, VkCommandPool commandPool) {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,  VkCommandPool command_pool, VkDevice device, VkQueue graphicsQueue) {
    VkCommandBuffer command_buffer = beginSingleTimeCommands(command_pool, device);
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){ 0, 0, 0 };
    region.imageExtent = (VkExtent3D){ width, height, 1 };
    
    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    endSingleTimeCommands(command_buffer, graphicsQueue, device, command_pool);
}

void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandPool commandPool, VkDevice device, VkQueue graphicsQueue) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool, device);
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        assert("unssopred layout transition");
        //throw std::invalid_argument("unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(commandBuffer, source_stage, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier );
    
    endSingleTimeCommands(commandBuffer, graphicsQueue, device, commandPool);
}

void create_image_view(VkImageView *image_view, VkImage image, VkFormat format, VkDevice device) {
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(device, &view_info, NULL, image_view);
    assert(result == VK_SUCCESS);
    
}

void create_texture_sampler(VkSampler *sampler, VkDevice device) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    VkResult result = vkCreateSampler(device, &samplerInfo, NULL, sampler);
    assert(result == VK_SUCCESS);
}

void create_texture_image(VkImage *image, VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue, VkPhysicalDeviceMemoryProperties physical_device_memory_properties, VkDeviceMemory *device_memory) {
    int width, height, channels;
    unsigned char *pixels = stbi_load("viking_room.png", &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize image_size = width * height * 4;
    assert(pixels);
    //assert ((VkDeviceSize)sizeof(pixels) == image_size); //TODO: this is temporary
    
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(&staging_buffer, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device, physical_device_memory_properties, &staging_buffer_memory);
    
    void *memory_pointer;
    vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &memory_pointer);
    memcpy(memory_pointer, pixels, (size_t)image_size);
    vkUnmapMemory(device, staging_buffer_memory);
    
    stbi_image_free(pixels);
    
    create_image(image, device_memory, 
                 width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device, physical_device_memory_properties);
    
    transition_image_layout(*image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command_pool, device, graphics_queue);
    
    copyBufferToImage(staging_buffer, *image, (uint32_t)width, (uint32_t)height, command_pool, device, graphics_queue);
    
    transition_image_layout(*image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_pool, device, graphics_queue);
    
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}

void init_vulkan() {
    VkResult result;
    
    //init glfw
    glfwInit();
    
    //get instance layers
    uint32_t instance_layer_count;
    vkEnumerateInstanceLayerProperties(&instance_layer_count ,NULL);
    VkLayerProperties *instance_layer_properties = NULL;
    arraddn(instance_layer_properties, instance_layer_count);
    result = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_properties);
    assert(result == VK_SUCCESS);
    
    //add debug layer
    char **instance_layer_names = NULL;
    arrput(instance_layer_names, "VK_LAYER_KHRONOS_validation");
    //TODO: check for validation layer
    
    //get instance extensions
    VkExtensionProperties *instance_extensions = NULL;
    for (int i = 0; i < instance_layer_count; ++i) {
        uint32_t instance_extensions_count = 0;
        vkEnumerateInstanceExtensionProperties(instance_layer_properties[i].layerName, &instance_extensions_count, NULL);
        if (instance_extensions_count) {
            //VkExtensionProperties *added_extensions = (VkExtensionProperties *)arraddn(instance_extensions, instance_extensions_count);
            VkExtensionProperties *added_extensions = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * instance_extensions_count);
            
            result = vkEnumerateInstanceExtensionProperties(instance_layer_properties[i].layerName, &instance_extensions_count, added_extensions);
            assert(result == VK_SUCCESS);
        }
    }
    
    //add surface extension
    //TODO: this is messy as fuck, look for nicer way to load into a dynamic array
    uint32_t instance_extension_names_count;
    glfwGetRequiredInstanceExtensions(&instance_extension_names_count);
    const char **_instance_extension_names = NULL;
    _instance_extension_names = glfwGetRequiredInstanceExtensions(&instance_extension_names_count);
    const char **instance_extension_names = NULL;
    for (int i = 0; i < instance_extension_names_count; ++i) {
        arrput(instance_extension_names, _instance_extension_names[i]);
    }
    
    //add debug extension
    //TODO: check if the extension even exists by comparing it to instance layer properties
    arrput(instance_extension_names, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    
    //create instance
    char *app_name = "app";
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = NULL;
    application_info.pApplicationName = app_name;
    application_info.applicationVersion = 1;
    application_info.pEngineName = app_name;
    application_info.engineVersion = 1;
    application_info.apiVersion = VK_API_VERSION_1_0; //TODO: CHANGE VERSIONS 
    
    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &application_info;
    instance_info.enabledLayerCount = 1;
    const char *arr[1] = {"VK_LAYER_KHRONOS_validation"};
    instance_info.ppEnabledLayerNames = arr;
    instance_info.enabledExtensionCount = arrlen(instance_extension_names);
    instance_info.ppEnabledExtensionNames = (const char * const *)instance_extension_names;
    
    VkInstance instance;
    result = vkCreateInstance(&instance_info, NULL, &instance);
    assert(result == VK_SUCCESS);
    
#if 0
    //init debug callback
    PFN_vkCreateDebugReportCallbackEXT debug_callback;
    PFN_vkDestroyDebugReportCallbackEXT debug_destroy_callback;
    VkDebugReportCallbackEXT debug_report_callback;
    
    debug_callback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    assert(debug_callback);
    
    debug_destroy_callback =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    assert(debug_destroy_callback);
    
    VkDebugReportCallbackCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debug_info.pNext = NULL;
    debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debug_info.pfnCallback = debug_callback;
    debug_info.pUserData = NULL;
    
    //result = dbgCreateDebugReportCallback(inst, &debug_info, NULL, &debug_report_callback);
#endif
    
    //get physical devices
    uint32_t physical_devices_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, NULL);
    assert(physical_devices_count);
    VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * physical_devices_count);
    result = vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices);
    assert(result == VK_SUCCESS);
    VkPhysicalDevice physical_device = physical_devices[0];
    
    //get queue families
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    assert(queue_family_count);
    VkQueueFamilyProperties *queue_family_properties = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);
    assert(result == VK_SUCCESS);
    
    //get device extensions
#if 0
    uint32_t device_extensions_count = 0;
    char *device_layer_name = NULL;
    vkEnumerateDeviceExtensionProperties(physical_device, device_layer_name, &device_extensions_count, NULL);
    VkExtensionProperties *device_extensions = (vkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * device_extensions_count);
    vkEnumerateDeviceExtensionProperties(physical_device, layer_name, &device_extensions_count, device_extensions);
    assert(result == VK_SUCCESS);
#endif
    char *device_extension_names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    
    //get memory properties
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    //get physical device properties
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    
    //init device extensions 
    //soon...
    
    //create window
    int window_width = 500;
    int window_height = 500;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, app_name, 0, 0);
    //glfwSetKeyCallback(window, keyCallback); TODO: this might be input callback from niagara
    
    //create surface
    VkSurfaceKHR surface;
    result = glfwCreateWindowSurface(instance, window, NULL, &surface);
    assert(result == VK_SUCCESS);
    
    //check for presentation queue
    //NOTE: validation layer forces me yo use the second version
#if 0
    uint32_t queue_family_index = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (glfwGetPhysicalDevicePresentationSupport(instance, physical_device, i) == GLFW_TRUE) {
            queue_family_index = i;
            break;
        }
    }
    assert(queue_family_index != UINT32_MAX);
#endif
    uint32_t queue_family_index = UINT32_MAX;
    VkBool32 presentation_support;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentation_support);
        if (presentation_support) {
            queue_family_index = i;
            break;
        }
    }
    assert(queue_family_index != UINT32_MAX);
    
    //check for format
    uint32_t format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
    assert(result == VK_SUCCESS);
    VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats);
    assert(result == VK_SUCCESS);
    VkFormat format;
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else {
        assert(format_count >= 1);
        format = surface_formats[0].format;
    }
    
    
    //init device (logical)
    VkDeviceQueueCreateInfo queue_info = {};
    
    float queue_priorities[1] = {0.0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;
    queue_info.queueFamilyIndex = queue_family_index;
    
    //NOTE: this got added from the vulkan  tutorial
    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = ARRAY_COUNT(device_extension_names);
    device_info.ppEnabledExtensionNames = (const char * const *)device_extension_names;
    device_info.pEnabledFeatures = &device_features;
    
    VkDevice device;
    result = vkCreateDevice(physical_device, &device_info, NULL, &device);
    assert(result == VK_SUCCESS);
    
    //init command pool
    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    command_pool_info.queueFamilyIndex = queue_family_index;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    VkCommandPool command_pool;
    result = vkCreateCommandPool(device, &command_pool_info, NULL, &command_pool);
    assert(result == VK_SUCCESS);
    
    //init command buffer
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = NULL;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    result = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer);
    assert(result == VK_SUCCESS);
    
    //execture begin command buffer
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = NULL;
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = NULL;
    
    result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    assert(result == VK_SUCCESS);
    
    //init device queue
    //TODO: consider making a different queue for graphics and presentation
    VkQueue queue;
    vkGetDeviceQueue(device, queue_family_index, 0, &queue);
    
    //init swapchain
    VkSurfaceCapabilitiesKHR surface_capabilities;
    
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    assert(result == VK_SUCCESS);
    
    uint32_t present_mode_count;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
    assert(result == VK_SUCCESS);
    
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(present_mode_count * sizeof(VkPresentModeKHR));
    assert(present_modes);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);
    assert(result == VK_SUCCESS);
    
    VkExtent2D swapchain_extent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surface_capabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchain_extent.width = window_width;
        swapchain_extent.height = window_height;
        if (swapchain_extent.width < surface_capabilities.minImageExtent.width) {
            swapchain_extent.width = surface_capabilities.minImageExtent.width;
        } else if (swapchain_extent.width > surface_capabilities.maxImageExtent.width) {
            swapchain_extent.width = surface_capabilities.maxImageExtent.width;
        }
        
        if (swapchain_extent.height < surface_capabilities.minImageExtent.height) {
            swapchain_extent.height = surface_capabilities.minImageExtent.height;
        } else if (swapchain_extent.height > surface_capabilities.maxImageExtent.height) {
            swapchain_extent.height = surface_capabilities.maxImageExtent.height;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchain_extent = surface_capabilities.currentExtent;
    }
    
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_create_info.imageFormat = format;
    swapchain_create_info.imageExtent.width = swapchain_extent.width;
    swapchain_create_info.imageExtent.height = swapchain_extent.height;
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    swapchain_create_info.clipped = VK_TRUE; //NOTE: for some reason it's false in the tutorial
    swapchain_create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //NOTE: if the presentation and graphics queue are different we'll beed to change this varaible
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = NULL;
    
    VkSwapchainKHR swapchain;
    result = vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain);
    assert(result == VK_SUCCESS);
    
    //init image views
    uint32_t swapchain_image_count;
    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL);
    assert(result == VK_SUCCESS);
    
    VkImage *swapchain_images = (VkImage *)malloc(swapchain_image_count * sizeof(VkImage));
    assert(swapchain_images);
    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);
    assert(result == VK_SUCCESS);
    
    VkImageView *swapchain_image_views = (VkImageView *)malloc(sizeof(VkImageView) * swapchain_image_count);
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = NULL;
        image_view_create_info.format = format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.flags = 0;
        image_view_create_info.image = swapchain_images[i];
        
        VkImageView image_view;
        result = vkCreateImageView(device, &image_view_create_info, NULL, &image_view);
        swapchain_image_views[i] = image_view;
        assert(result == VK_SUCCESS);
    }
    
    //free(swapchain_images); TODO: not so sure about it
    uint32_t current_buffer = 0;
    
    if (NULL != present_modes) {
        free(present_modes);
    }
    
    //init depth buffer
    VkImageCreateInfo image_create_info = {};
    VkFormatProperties format_properties; //TODO: didnt I do it earlier?
    VkFormat depth_buffer_format = VK_FORMAT_D16_UNORM; //TODO: maybe go for a different format 
    vkGetPhysicalDeviceFormatProperties(physical_device, depth_buffer_format, &format_properties);
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL; //TODO: better check what type to use
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = depth_buffer_format;
    image_create_info.extent.width = window_width;
    image_create_info.extent.height = window_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = NUM_SAMPLES;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.queueFamilyIndexCount = 0; //TODO: the tutorial defines this as 0 without checking for other queue families
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.flags = 0;
    VkImage depth_image;
    result = vkCreateImage(device, &image_create_info, NULL, &depth_image);
    assert(result == VK_SUCCESS);
    
    VkMemoryAllocateInfo depth_allocate_info = {};
    depth_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth_allocate_info.pNext = NULL;
    depth_allocate_info.allocationSize = 0;
    depth_allocate_info.memoryTypeIndex = 0;
    VkMemoryRequirements depth_memory_requirements;
    vkGetImageMemoryRequirements(device, depth_image, &depth_memory_requirements);
    depth_allocate_info.allocationSize = depth_memory_requirements.size;
    //TODO: take another look at this function
    get_memory_type(physical_device_memory_properties, depth_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_allocate_info.memoryTypeIndex);
    VkDeviceMemory depth_memory;
    result = vkAllocateMemory(device, &depth_allocate_info, NULL, &depth_memory);
    assert(result == VK_SUCCESS);
    result = vkBindImageMemory(device, depth_image, depth_memory, 0);
    assert(result == VK_SUCCESS);
    
    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = NULL;
    image_view_info.image = VK_NULL_HANDLE;
    image_view_info.format = depth_buffer_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.flags = 0;
    image_view_info.image = depth_image;
    //NOTE: de fuck
    if (depth_buffer_format == VK_FORMAT_D16_UNORM_S8_UINT || depth_buffer_format == VK_FORMAT_D24_UNORM_S8_UINT ||
        depth_buffer_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        image_view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VkImageView depth_buffer_image_view;
    result = vkCreateImageView(device, &image_view_info, NULL, &depth_buffer_image_view);
    assert(result == VK_SUCCESS);
    
    //init uniform buffer
#if 0
    float fov = glm_rad(45.0f);
    if (window_width > window_height) {
        fov *= (float)window_height / (float)window_width;
    }
    mat4 projection_matrix;
    glm_perspective(fov, (float)window_height / (float)window_width, 0.1f, 100.0f, projection_matrix);
    mat4 view_matrix;
    //glm_mat4_identity(view_matrix);
    glm_lookat((vec3){-5, 3, -10}, (vec3){0, 0, 0}, (vec3){0, -1, 0}, view_matrix);
    glm_mat4_identity(view_matrix);
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    // Vulkan clip space has inverted Y and half Z, fuck you vulkan...
    mat4 clip_matrix = (mat4){ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f };
    
    mat4 mvp_matrix;
    glm_mat4_mulN((mat4 *[]){ &clip_matrix, &projection_matrix, &view_matrix, &model_matrix}, 4, mvp_matrix);
#else
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    //glm_rotate(model_matrix, f), (vec3){0, 0, 0.1f}); 
    mat4 view_matrix;
    glm_lookat((vec3){2.0f, 2.0f, 2.0f}, (vec3){0, 0, 0}, (vec3){0, 0, 1.0f}, view_matrix);
    mat4 projection_matrix;
    glm_perspective(glm_rad(45.0f), (float)(window_width / window_height), 0.1f, 100.0f, projection_matrix);
    projection_matrix[1][1] *= -1;
    mat4 mvp_matrix;
    glm_mat4_mulN((mat4 *[]){&projection_matrix, &view_matrix, &model_matrix}, 3, mvp_matrix);
#endif
    
    VkBufferCreateInfo uniform_buffer_create_info = {};
    uniform_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buffer_create_info.pNext = NULL;
    uniform_buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniform_buffer_create_info.size = sizeof(mvp_matrix); //TODO: am I sure about this
    uniform_buffer_create_info.queueFamilyIndexCount = 0;
    uniform_buffer_create_info.pQueueFamilyIndices = NULL;
    uniform_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    uniform_buffer_create_info.flags = 0;
    VkBuffer uniform_buffer;
    result = vkCreateBuffer(device, &uniform_buffer_create_info, NULL, &uniform_buffer);
    assert(result == VK_SUCCESS);
    
    VkMemoryRequirements uniform_buffer_memory_requirements;
    vkGetBufferMemoryRequirements(device, uniform_buffer, &uniform_buffer_memory_requirements);
    VkMemoryAllocateInfo uniform_buffer_allocate_info = {};
    uniform_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniform_buffer_allocate_info.pNext = NULL;
    uniform_buffer_allocate_info.memoryTypeIndex = 0;
    uniform_buffer_allocate_info.allocationSize = uniform_buffer_memory_requirements.size;
    get_memory_type(physical_device_memory_properties, uniform_buffer_memory_requirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &uniform_buffer_allocate_info.memoryTypeIndex);
    //assert(pass && "No mappable, coherent memory");
    VkDeviceMemory uniform_buffer_device_memory;
    result = vkAllocateMemory(device, &uniform_buffer_allocate_info, NULL, &uniform_buffer_device_memory);
    assert(result == VK_SUCCESS);
    uint8_t *pData;
    result = vkMapMemory(device, uniform_buffer_device_memory, 0, uniform_buffer_memory_requirements.size, 0, (void **)&pData);
    assert(result == VK_SUCCESS);
    memcpy(pData, &mvp_matrix, sizeof(mvp_matrix));
    vkUnmapMemory(device, uniform_buffer_device_memory);
    result = vkBindBufferMemory(device, uniform_buffer, uniform_buffer_device_memory, 0);
    assert(result == VK_SUCCESS);
    VkDescriptorBufferInfo uniform_buffer_descriptor_buffer_info;
    uniform_buffer_descriptor_buffer_info.buffer = uniform_buffer;
    uniform_buffer_descriptor_buffer_info.offset = 0;
    uniform_buffer_descriptor_buffer_info.range = sizeof(mvp_matrix);
    
    //init renderpass
    /* Need attachments for render target and depth buffer */
    bool clear = true; //TODO: clean that up too
    VkAttachmentDescription attachments[2];
    attachments[0].format = format;
    attachments[0].samples = NUM_SAMPLES;
    attachments[0].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;;
    attachments[0].flags = 0;
    
    bool include_depth = true; //TODO: change this later on 
    if (include_depth) {
        attachments[1].format = depth_buffer_format;
        attachments[1].samples = NUM_SAMPLES;
        attachments[1].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].flags = 0;
    }
    
    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depth_attachment_reference = {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.flags = 0;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = NULL;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pResolveAttachments = NULL;
    subpass_description.pDepthStencilAttachment = include_depth ? &depth_attachment_reference : NULL;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = NULL;
    
    // Subpass dependency to wait for wsi image acquired semaphore before starting layout transition
    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;
    
    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = NULL;
    render_pass_create_info.attachmentCount = include_depth ? 2 : 1;
    render_pass_create_info.pAttachments = attachments;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;
    VkRenderPass render_pass;
    result = vkCreateRenderPass(device, &render_pass_create_info, NULL, &render_pass);
    assert(result == VK_SUCCESS);
    
    //load shaders
    //TODO: maybe the length out parameter is not as pretty as I would like it to be
    size_t vertex_shader_length;
    const char *vertex_shader_code = read_file("vertex_shader.spv",  &vertex_shader_length);
    VkShaderModuleCreateInfo vertex_shader_module_create_info = {};
    vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_create_info.codeSize = vertex_shader_length;
    vertex_shader_module_create_info.pCode = (const uint32_t *) vertex_shader_code;
    VkShaderModule vertex_shader_module;
    result = vkCreateShaderModule(device, &vertex_shader_module_create_info, NULL, &vertex_shader_module);
    assert(result == VK_SUCCESS);
    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {};
    vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vertex_shader_module;
    vertex_shader_stage_create_info.pName = "main";
    
    size_t fragment_shader_length;
    const char *fragment_shader_code = read_file("fragment_shader.spv", &fragment_shader_length);
    VkShaderModuleCreateInfo fragment_shader_module_create_info = {};
    fragment_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_create_info.codeSize = fragment_shader_length;
    fragment_shader_module_create_info.pCode = (const uint32_t *) fragment_shader_code;
    VkShaderModule fragment_shader_module;
    result = vkCreateShaderModule(device, &fragment_shader_module_create_info, NULL, &fragment_shader_module);
    assert(result == VK_SUCCESS);
    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info = {};
    fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = fragment_shader_module;
    fragment_shader_stage_create_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_create_info, fragment_shader_stage_create_info };
    
    //init framebuffers
    VkFramebuffer *framebuffers = (VkFramebuffer *)malloc(swapchain_image_count * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = (VkImageView[]){ swapchain_image_views[i], depth_buffer_image_view };
        framebuffer_create_info.width = window_width;
        framebuffer_create_info.height = window_height;
        framebuffer_create_info.layers = 1;
        
        result = vkCreateFramebuffer(device, &framebuffer_create_info, NULL, framebuffers + i);
        assert(result == VK_SUCCESS);
    }
    
    //init image
    VkImage image;
    VkDeviceMemory image_device_memory;
    create_texture_image(&image, device, command_pool, queue, physical_device_memory_properties, &image_device_memory);
    
    //init texture image views
    //TODO: am i sure about the format?
    VkImageView texture_image_view;
    VkDeviceMemory texture_image_memory;
    create_image_view(&texture_image_view, image, VK_FORMAT_R8G8B8A8_SRGB, device);
    
    //init texture sampler
    //TODO: pull this out of the function
    VkSampler texture_sampler;
    create_texture_sampler(&texture_sampler, device);
    
    //load model
    const char *path = "viking_room.obj";
    fastObjMesh* obj = fast_obj_read(path);
    assert(obj);
    vertex vertex_data[obj->face_count * 3];
    for (int i = 0; i < obj->face_count * 3; ++i) {
        vertex_data[i].pos[0] = obj->positions[obj->indices[i].p * 3 + 0];
        vertex_data[i].pos[1] = obj->positions[obj->indices[i].p * 3 + 1];
        vertex_data[i].pos[2] = obj->positions[obj->indices[i].p * 3 + 2];
        
        vertex_data[i].textcoords[0] = obj->texcoords[obj->indices[i].t * 2 + 0];
        vertex_data[i].textcoords[1] = 1 - obj->texcoords[obj->indices[i].t * 2 + 1];
        
        vertex_data[i].color[0] = 1.0f;
        vertex_data[i].color[1] = 1.0f;
        vertex_data[i].color[2] = 1.0f;
    }
    
    uint32_t index_data[] = {1};
    //uint32_t *index_data = obj->indices;
    size_t vertex_data_size = sizeof(vertex_data);
    
    //init vertex buffer%
    VkBuffer vertex_staging_buffer;
    VkDeviceMemory vertex_staging_buffer_device_memory;
    create_buffer(&vertex_staging_buffer, vertex_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device, physical_device_memory_properties, &vertex_staging_buffer_device_memory);
    void *vertex_staging_buffer_data_pointer;
    result = vkMapMemory(device, vertex_staging_buffer_device_memory, 0, vertex_data_size, 0, &vertex_staging_buffer_data_pointer);
    memcpy(vertex_staging_buffer_data_pointer, vertex_data, vertex_data_size);
    vkUnmapMemory(device, vertex_staging_buffer_device_memory);
    
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_device_memory;
    create_buffer(&vertex_buffer, vertex_data_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT , device, physical_device_memory_properties,  &vertex_buffer_device_memory);
    copy_buffer(vertex_staging_buffer, vertex_buffer, vertex_data_size, command_pool, device, queue);
    
    vkDestroyBuffer(device, vertex_staging_buffer, NULL);
    vkFreeMemory(device, vertex_staging_buffer_device_memory, NULL);
    
    //init index buffer
    VkBuffer index_staging_buffer;
    VkDeviceMemory index_staging_buffer_device_memory;
    create_buffer(&index_staging_buffer, sizeof(index_data), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device,physical_device_memory_properties, &index_staging_buffer_device_memory);
    
    void *index_staging_buffer_data_pointer;
    result = vkMapMemory(device, index_staging_buffer_device_memory, 0, sizeof(index_data), 0, &index_staging_buffer_data_pointer);
    assert(result == VK_SUCCESS);
    memcpy(index_staging_buffer_data_pointer, index_data, sizeof(index_data));
    vkUnmapMemory(device, index_staging_buffer_device_memory);
    
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_device_memory;
    create_buffer(&index_buffer, sizeof(index_data),  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device, physical_device_memory_properties,
                  &index_buffer_device_memory);
    copy_buffer(index_staging_buffer, index_buffer, sizeof(index_data), command_pool, device, queue);
    
    vkDestroyBuffer(device, index_staging_buffer, NULL);
    vkFreeMemory(device, index_staging_buffer_device_memory, NULL);
    
    //init vertex input binding descriptions
    VkVertexInputBindingDescription vertex_input_binding_description = {};
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_binding_description.stride = sizeof(vertex);
    
    //init vertex input attribute descriptions
    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3] = {};
    
    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = offsetof(struct vertex, pos);
    
    vertex_input_attribute_descriptions[1].binding = 0;
    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].format =  VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[1].offset = offsetof(struct vertex, color);
    
    vertex_input_attribute_descriptions[2].binding = 0;
    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[2].offset = offsetof(struct vertex, textcoords);
    
    //init descriptor set layouts
    VkDescriptorSetLayoutBinding uniform_descriptor_set_layout_binding = {};
    uniform_descriptor_set_layout_binding.binding = 0;
    uniform_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_descriptor_set_layout_binding.descriptorCount = 1;
    uniform_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uniform_descriptor_set_layout_binding.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutBinding sampler_descriptor_set_layout_binding = {};
    sampler_descriptor_set_layout_binding.binding = 1;
    sampler_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_descriptor_set_layout_binding.descriptorCount = 1;
    sampler_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampler_descriptor_set_layout_binding.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutBinding bindings[] = {uniform_descriptor_set_layout_binding, sampler_descriptor_set_layout_binding};
    
    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.flags = 0; 
    descriptor_layout.bindingCount = 2;
    descriptor_layout.pBindings = bindings;
    VkDescriptorSetLayout descriptor_set_layout;
    result = vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, &descriptor_set_layout);
    assert(result == VK_SUCCESS);
    
    //init pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = NULL;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = NULL;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout);
    assert(result == VK_SUCCESS);
    
    //init cache pipeline
    VkPipelineCacheCreateInfo pipeline_cache_create_info;
    pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_create_info.pNext = NULL;
    pipeline_cache_create_info.initialDataSize = 0;
    pipeline_cache_create_info.pInitialData = NULL;
    pipeline_cache_create_info.flags = 0;
    VkPipelineCache pipeline_cache;
    result = vkCreatePipelineCache(device, &pipeline_cache_create_info, NULL, &pipeline_cache);
    assert(result == VK_SUCCESS);
    
    //init pipeline
    //TODO: apparently this constant got deprecated so fix it...
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = NULL;
    dynamicState.pDynamicStates = dynamicStateEnables;
    dynamicState.dynamicStateCount = 0;
    
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {};
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1; 
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = ARRAY_COUNT(vertex_input_attribute_descriptions);
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = (const VkVertexInputAttributeDescription *)&vertex_input_attribute_descriptions;
    
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info;
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.pNext = NULL;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info;
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.pNext = NULL;
    rasterization_state_create_info.flags = 0;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0;
    rasterization_state_create_info.depthBiasClamp = 0;
    rasterization_state_create_info.depthBiasSlopeFactor = 0;
    rasterization_state_create_info.lineWidth = 1.0f;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info;
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments = &color_blend_attachment;
    pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;
    
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.pNext = NULL;
    pipeline_viewport_state_create_info.flags = 0;
    pipeline_viewport_state_create_info.viewportCount = NUM_VIEWPORTS;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    pipeline_viewport_state_create_info.scissorCount = NUM_SCISSORS;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    pipeline_viewport_state_create_info.pScissors = NULL;
    pipeline_viewport_state_create_info.pViewports = NULL;
    
    
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {};
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    
    VkGraphicsPipelineCreateInfo pipeline_create_info;
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = NULL;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = 0;
    pipeline_create_info.flags = 0;
    pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    pipeline_create_info.pTessellationState = NULL;
    pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    pipeline_create_info.pDynamicState = &dynamicState;
    pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;
    VkPipeline pipeline;
    result = vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_create_info, NULL, &pipeline);
    assert(result == VK_SUCCESS);
    
    //init descriptor pool
    VkDescriptorPoolSize type_count[2];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].descriptorCount = 1;
    type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    type_count[1].descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo decriptor_pool_create_info = {};
    decriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    decriptor_pool_create_info.pNext = NULL;
    decriptor_pool_create_info.maxSets = 1;
    decriptor_pool_create_info.poolSizeCount = 2;
    decriptor_pool_create_info.pPoolSizes = type_count;
    VkDescriptorPool descriptor_pool;
    result = vkCreateDescriptorPool(device, &decriptor_pool_create_info, NULL, &descriptor_pool);
    assert(result == VK_SUCCESS);
    
    //init descriptor set
    //NOTE: vulkan tutorial uses a descriptor set for image swapchain image
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info[1];
    descriptor_set_allocate_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info[0].pNext = NULL;
    descriptor_set_allocate_info[0].descriptorPool = descriptor_pool;
    descriptor_set_allocate_info[0].descriptorSetCount = NUM_DESCRIPTOR_SETS;
    descriptor_set_allocate_info[0].pSetLayouts = &descriptor_set_layout;
    
    VkDescriptorSet descriptor_sets[NUM_DESCRIPTOR_SETS];
    result = vkAllocateDescriptorSets(device, descriptor_set_allocate_info, descriptor_sets);
    assert(result == VK_SUCCESS);
    
    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = texture_image_view;
    image_info.sampler = texture_sampler;
    
    VkWriteDescriptorSet write_descriptor_sets[2];
    
    write_descriptor_sets[0] = (VkWriteDescriptorSet){};
    write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[0].pNext = NULL;
    write_descriptor_sets[0].dstSet = descriptor_sets[0];
    write_descriptor_sets[0].descriptorCount = 1;
    write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_sets[0].pBufferInfo = &uniform_buffer_descriptor_buffer_info;
    write_descriptor_sets[0].dstArrayElement = 0;
    write_descriptor_sets[0].dstBinding = 0;
    
    write_descriptor_sets[1] = (VkWriteDescriptorSet){};
    write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[1].dstSet = descriptor_sets[0];
    write_descriptor_sets[1].dstBinding = 1;
    write_descriptor_sets[1].descriptorCount = 1;
    write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_sets[1].pImageInfo = &image_info;
    write_descriptor_sets[1].dstArrayElement = 0;
    
    vkUpdateDescriptorSets(device, ARRAY_COUNT(write_descriptor_sets), write_descriptor_sets, 0, NULL);
    
    //this is where initializaiton ends
    
    /* VULKAN_KEY_START */
    
    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    
    VkSemaphore imageAcquiredSemaphore;
    VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
    imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    imageAcquiredSemaphoreCreateInfo.pNext = NULL;
    imageAcquiredSemaphoreCreateInfo.flags = 0;
    
    result = vkCreateSemaphore(device, &imageAcquiredSemaphoreCreateInfo, NULL, &imageAcquiredSemaphore);
    assert(result == VK_SUCCESS);
    
    // Get the index of the next available swapchain image:
    uint32_t image_index;
    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE,
                                   &image_index);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(result == VK_SUCCESS);
    
    VkRenderPassBeginInfo render_pass_begin;
    render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin.pNext = NULL;
    render_pass_begin.renderPass = render_pass;
    render_pass_begin.framebuffer = framebuffers[image_index];
    render_pass_begin.renderArea.offset.x = 0;
    render_pass_begin.renderArea.offset.y = 0;
    render_pass_begin.renderArea.extent.width = window_width;
    render_pass_begin.renderArea.extent.height = window_height;
    render_pass_begin.clearValueCount = 2;
    render_pass_begin.pClearValues = clear_values;
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, NUM_DESCRIPTOR_SETS,
                            (const VkDescriptorSet *)descriptor_sets, 0, NULL);
    
    const VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    
    VkViewport viewport;
#ifdef __ANDROID__
    // Disable dynamic viewport on Android. Some drive has an issue with the dynamic viewport
    // feature.
#else
    viewport.height = (float)window_height;
    viewport.width = (float)window_width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(command_buffer, 0, NUM_VIEWPORTS, &viewport);
#endif
    
    VkRect2D scissor;
#ifdef __ANDROID__
    // Disable dynamic viewport on Android. Some drive has an issue with the dynamic scissors
    // feature.
#else
    scissor.extent.width = window_width;
    scissor.extent.height = window_height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(command_buffer, 0, NUM_SCISSORS, &scissor);
#endif
    
    vkCmdDraw(command_buffer, obj->face_count * 3, 1, 0, 0);
    //vkCmdDrawIndexed(command_buffer, ARRAY_COUNT(index_data), 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);
    result = vkEndCommandBuffer(command_buffer);
    const VkCommandBuffer command_buffers[] = { command_buffer }; //NOTE: why would we need more than one command buffer?
    VkFenceCreateInfo fence_create_info;
    VkFence draw_fence;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;
    vkCreateFence(device, &fence_create_info, NULL, &draw_fence);
    
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    submit_info[0].pWaitSemaphores = &imageAcquiredSemaphore;
    submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = (const VkCommandBuffer *)command_buffers;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;
    
    /* Queue the command buffer for execution */
    result = vkQueueSubmit(queue, 1, submit_info, draw_fence);
    assert(result == VK_SUCCESS);
    
    /* Now present the image in the window */
    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &image_index;
    present.pWaitSemaphores = NULL;
    present.waitSemaphoreCount = 0;
    present.pResults = NULL;
    
    /* Make sure command buffer is finished before presenting */
    do {
        result = vkWaitForFences(device, 1, &draw_fence, VK_TRUE, FENCE_TIMEOUT);
    } while (result == VK_TIMEOUT);
    
    assert(result == VK_SUCCESS);
    result = vkQueuePresentKHR(queue, &present);
    assert(result == VK_SUCCESS);
    
    usleep(1);
    /* VULKAN_KEY_END */
    //if (info.save_images) write_ppm(info, "15-draw_cube");
    
    //cleanup
    
}

int main() {
    init_vulkan();
    while(1);
    return 0;
}
