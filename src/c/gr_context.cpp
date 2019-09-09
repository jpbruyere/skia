/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkSurface.h"
#include "include/gpu/GrContext.h"
#include "include/gpu/vk/GrVkBackendContext.h"
#include "include/gpu/vk/GrVkTypes.h"

#include "include/c/gr_context.h"
#include "src/c/sk_types_priv.h"

PFN_vkVoidFunction getProcFunc (const char * function_name, VkInstance instance, VkDevice device) {
	if (device != VK_NULL_HANDLE)
		return vkGetDeviceProcAddr(device, function_name);
	return vkGetInstanceProcAddr(instance, function_name);
}

gr_context_t* gr_context_new_vulkan(VkInstance inst, VkPhysicalDevice phy, VkDevice dev, VkQueue queue)
{
	GrVkBackendContext vkContext = {};// = GrVkBackendContext::create new GrVkBackendContext;
	vkContext.fInstance = inst;
	vkContext.fPhysicalDevice = phy;
	vkContext.fQueue = queue;
	vkContext.fDevice = dev;
	vkContext.fGraphicsQueueIndex = 0;
	vkContext.fOwnsInstanceAndDevice = false;
	vkContext.fGetProc = GrVkGetProc(getProcFunc);

	sk_sp<GrContext> ctx = GrContext::MakeVulkan(vkContext);

	return (gr_context_t*)ctx.release();
}


void gr_context_unref(gr_context_t* ctx) {
	SkSafeUnref((GrContext*)ctx);
}
void gr_context_flush (gr_context_t* ctx){
	GrContext* gr = reinterpret_cast<GrContext*>(ctx);
	gr->flush();
}

sk_surface_t* sk_surface_make_rendertarget(gr_context_t* ctx, const sk_imageinfo_t* cinfo) {
	GrContext* gr = reinterpret_cast<GrContext*>(ctx);
	const SkImageInfo* info = reinterpret_cast<const SkImageInfo*>(cinfo);
	return (sk_surface_t*)SkSurface::MakeRenderTarget(gr,SkBudgeted::kNo, *info).release();
}

gr_vk_image_info_t* sk_surface_get_vk_image_info(sk_surface_t* csurf) {
	SkSurface* surf = (SkSurface*)csurf;
	GrBackendTexture tex = surf->getBackendTexture(SkSurface::BackendHandleAccess::kFlushRead_BackendHandleAccess);
	GrVkImageInfo* vkII = new GrVkImageInfo();
	tex.getVkImageInfo(vkII);
	return (gr_vk_image_info_t*)vkII;
}
void gr_vk_image_info_delete (gr_vk_image_info_t* cinfo) {
	GrVkImageInfo* info = reinterpret_cast<GrVkImageInfo*>(cinfo);
	delete info;
}

VkImage gr_vk_image_info_get_image (gr_vk_image_info_t* vkImgInfo) {
	GrVkImageInfo* vkII = reinterpret_cast<GrVkImageInfo*>(vkImgInfo);
	return vkII->fImage;
}
VkImageTiling gr_vk_image_info_get_tiling (gr_vk_image_info_t* vkImgInfo) {
	GrVkImageInfo* vkII = reinterpret_cast<GrVkImageInfo*>(vkImgInfo);
	return vkII->fImageTiling;
}
VkImageLayout gr_vk_image_info_get_layout (gr_vk_image_info_t* vkImgInfo) {
	GrVkImageInfo* vkII = reinterpret_cast<GrVkImageInfo*>(vkImgInfo);
	return vkII->fImageLayout;
}

