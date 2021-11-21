// EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL
// DO NOT USE -- FOR INTERNAL TESTING ONLY

#ifndef gr_context_DEFINED
#define gr_context_DEFINED

#include "include/c/sk_types.h"

#include "vulkan/vulkan.h"

SK_C_PLUS_PLUS_BEGIN_GUARD

SK_API gr_context_t* gr_context_new_vulkan(VkInstance inst, VkPhysicalDevice phy, VkDevice dev, VkQueue queue);

SK_API void gr_context_unref(gr_context_t*);
SK_API void gr_context_flush(gr_context_t* ctx);

SK_API sk_surface_t* sk_surface_make_rendertarget(gr_context_t* ctx, const sk_imageinfo_t* cinfo);

SK_API gr_vk_image_info_t *sk_surface_get_vk_image_info(sk_surface_t* csurf);

SK_API void				gr_vk_image_info_delete		(gr_vk_image_info_t* cinfo);
SK_API VkImage			gr_vk_image_info_get_image	(gr_vk_image_info_t* vkImgInfo);
SK_API VkImageTiling	gr_vk_image_info_get_tiling (gr_vk_image_info_t* vkImgInfo);
SK_API VkImageLayout	gr_vk_image_info_get_layout (gr_vk_image_info_t* vkImgInfo);

SK_C_PLUS_PLUS_END_GUARD

#endif
