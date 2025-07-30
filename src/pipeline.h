#ifndef PIPELINE_H
#define PIPELINE_H

#include <SDL3/SDL.h>

bool Pipeline_Init();
bool Pipeline_Unanimated_Init();
bool Pipeline_BoneAnimated_Init();
bool Pipeline_RigidAnimated_Init();
bool Pipeline_Instanced_Init();
bool Pipeline_Text_Init();

SDL_GPUComputePipeline* Pipeline_Compute_Init
(
	SDL_GPUDevice* gpu_device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
);

#endif // PIPELINE_H