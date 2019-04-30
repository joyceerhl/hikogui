// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "PipelineImage_Image.hpp"
#include "PipelineImage_TextureMap.hpp"
#include "PipelineImage_Page.hpp"
#include "TTauri/Draw/PixelMap.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace TTauri::GUI {
class Device_vulkan;
}

namespace TTauri::GUI::PipelineImage {

struct DeviceShared final {
    struct Error : virtual boost::exception, virtual std::exception {};

    static const size_t atlasNrHorizontalPages = 60;
    static const size_t atlasNrVerticalPages = 60;
    static const size_t atlasImageWidth = atlasNrHorizontalPages * Page::widthIncludingBorder;
    static const size_t atlasImageHeight = atlasNrVerticalPages * Page::heightIncludingBorder;
    static const size_t atlasNrPagesPerImage = atlasNrHorizontalPages * atlasNrVerticalPages;
    static const size_t atlasMaximumNrImages = 16;
    static const size_t stagingImageWidth = 2048;
    static const size_t stagingImageHeight = 1024;

    std::weak_ptr<Device_vulkan> device;

    vk::Buffer indexBuffer;
    VmaAllocation indexBufferAllocation = {};

    vk::ShaderModule vertexShaderModule;
    vk::ShaderModule fragmentShaderModule;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    TextureMap stagingTexture;
    std::vector<TextureMap> atlasTextures;

    std::vector<Page> atlasFreePages;
    std::array<vk::DescriptorImageInfo, atlasMaximumNrImages> atlasDescriptorImageInfos;
    vk::Sampler atlasSampler;
    vk::DescriptorImageInfo atlasSamplerDescriptorImageInfo;

    std::unordered_map<std::string, std::shared_ptr<Image>> viewImages;

    DeviceShared(const std::shared_ptr<Device_vulkan> device);
    ~DeviceShared();

    DeviceShared(const DeviceShared &) = delete;
    DeviceShared &operator=(const DeviceShared &) = delete;
    DeviceShared(DeviceShared &&) = delete;
    DeviceShared &operator=(DeviceShared &&) = delete;

    /*! Deallocate vulkan resources.
    * This is called in the destructor of Device_vulkan, therefor we can not use our `std::weak_ptr<Device_vulkan> device`.
    */
    void destroy(gsl::not_null<Device_vulkan *> vulkanDevice);

    /*! Get the coordinate in the atlast from a page index.
     * \param page number in the atlas
     * \return x, y pixel coordine in an atlasTexture and z the atlasTextureIndex.
     */
    static glm::u64vec3 getAtlasPositionFromPage(Page page) {
        auto const imageIndex = page.nr / atlasNrPagesPerImage;
        auto const pageNrInsideImage = page.nr % atlasNrPagesPerImage;

        auto const pageY = pageNrInsideImage / atlasNrVerticalPages;
        auto const pageX = pageNrInsideImage % atlasNrVerticalPages;

        auto const x = pageX * Page::widthIncludingBorder + Page::border;
        auto const y = pageY * Page::heightIncludingBorder + Page::border;

        return {x, y, imageIndex};
    }

    std::vector<Page> getFreePages(size_t const nrPages);

    std::shared_ptr<Image> retainImage(const std::string &key, u64extent2 extent);
    void releaseImage(const std::shared_ptr<Image> &image);

    /*! Exchange an image when the key is different.
     * \param image A shared pointer to an image, which may be reseated.
     * \param key of the image.
     * \param extent of the image.
     */
    void exchangeImage(std::shared_ptr<Image> &image, const std::string &key, const u64extent2 extent);

    void drawInCommandBuffer(vk::CommandBuffer &commandBuffer);

    TTauri::Draw::PixelMap<uint32_t> getStagingPixelMap();
    void updateAtlasWithStagingPixelMap(const Image &image);
    void prepareAtlasForRendering();

private:
    void buildIndexBuffer();
    void teardownIndexBuffer(gsl::not_null<Device_vulkan *> vulkanDevice);
    void buildShaders();
    void teardownShaders(gsl::not_null<Device_vulkan *> vulkanDevice);
    void addAtlasImage();
    void buildAtlas();
    void teardownAtlas(gsl::not_null<Device_vulkan *> vulkanDevice);
};

}