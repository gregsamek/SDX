#include "shader.h"
#include "globals.h"

SDL_GPUShader* Shader_Load
(
	SDL_GPUDevice* gpu_device,
	const char* shaderFilename,
	Uint32 num_samplers,
	Uint32 num_storage_textures,
	Uint32 num_storage_buffers,
	Uint32 num_uniform_buffers
) 
{
	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Invalid shader stage: %s", shaderFilename);
		return NULL;
	}

	char fullPath[512];
	SDL_GPUShaderFormat supported_shader_formats = SDL_GetGPUShaderFormats(gpu_device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (supported_shader_formats & SDL_GPU_SHADERFORMAT_SPIRV) 
    {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/%s.spv", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} 
    else if (supported_shader_formats & SDL_GPU_SHADERFORMAT_MSL) 
    {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/%s.msl", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} 
    else if (supported_shader_formats & SDL_GPU_SHADERFORMAT_DXIL) 
    {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/%s.dxil", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} 
    else 
    {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "No valid backend shader format (SPIRV, MSL, DXIL)");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load shader from disk! %s", fullPath);
		return NULL;
	}

	// TODO switch to reflection to load json shader info e.g. num samplers, num uniform buffers, etc.
	// instead of hardcoding into calls of Shader_Load()

	SDL_GPUShaderCreateInfo shaderInfo = 
    {
		.code = (const Uint8*) code,
		.code_size = codeSize,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = num_samplers,
		.num_storage_textures = num_storage_textures,
		.num_storage_buffers = num_storage_buffers,
		.num_uniform_buffers = num_uniform_buffers
	};
	SDL_GPUShader* shader = SDL_CreateGPUShader(gpu_device, &shaderInfo);
	if (shader == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}

