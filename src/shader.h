#ifndef SHADER_H
#define SHADER_H

#include <SDL3/SDL.h>

SDL_GPUShader* Shader_Load
(
	SDL_GPUDevice* gpu_device,
	const char* shaderFilename,
	Uint32 num_samplers,
	Uint32 num_storage_textures,
	Uint32 num_storage_buffers,
	Uint32 num_uniform_buffers
);

#endif // SHADER_H