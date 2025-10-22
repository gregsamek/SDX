#ifndef PIPELINE_H
#define PIPELINE_H

#include <SDL3/SDL.h>

bool Pipeline_Init();
bool Pipeline_Unlit_Unanimated_Init();
bool Pipeline_BlinnPhong_Unanimated_Init();
bool Pipeline_PBR_Unanimated_Init();
bool Pipeline_PBR_Animated_Init();
bool Pipeline_RigidAnimated_Init();
bool Pipeline_Instanced_Init();
bool Pipeline_Text_Init();
bool Pipeline_Swapchain_Init();
bool Pipeline_Sprite_Init();

SDL_GPUComputePipeline* Pipeline_Compute_Init
(
	SDL_GPUDevice* gpu_device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
);

#endif // PIPELINE_H