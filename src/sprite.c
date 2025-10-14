#include "sprite.h"
#include "globals.h"
#include "texture.h"

// TODO switch to sprite sheets, add UV coordinates to Sprite struct
// (current implementation uses a dedicated texture for each sprite)

// TODO animated sprites

static bool Sprite_Load(const char* sprite_name, Sprite* sprite)
{
    SDL_Surface* texture_surface = LoadImage(sprite_name);
    if (!texture_surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture from URI: %s", sprite_name);
        return false;
    }
    sprite->texture = SDL_CreateGPUTexture(gpu_device, &(SDL_GPUTextureCreateInfo)
    {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
        .width = (Uint32)texture_surface->w,
        .height = (Uint32)texture_surface->h,
        .layer_count_or_depth = 1,
        .num_levels = n_mipmap_levels, 
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET // COLOR_TARGET is needed for mipmap generation
    });
    if (sprite->texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create main texture: %s", SDL_GetError());
        SDL_DestroySurface(texture_surface);
        return false;
    }

    sprite->aspect_ratio = (float)texture_surface->w / (float)texture_surface->h;

    Uint32 texture_data_size = (Uint32)texture_surface->w * texture_surface->h * 4;
    SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = texture_data_size
        }
    );
    if (texture_transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create texture transfer buffer: %s", SDL_GetError());
        SDL_DestroySurface(texture_surface);
        SDL_ReleaseGPUTexture(gpu_device, sprite->texture);
        return false;
    }

    Uint8* texture_transfer_mapped = SDL_MapGPUTransferBuffer(gpu_device, texture_transfer_buffer, false);
    if (texture_transfer_mapped == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map texture transfer buffer: %s", SDL_GetError());
        SDL_DestroySurface(texture_surface);
        SDL_ReleaseGPUTexture(gpu_device, sprite->texture);
        SDL_ReleaseGPUTransferBuffer(gpu_device, texture_transfer_buffer);
        return false;
    }

    SDL_memcpy(texture_transfer_mapped, texture_surface->pixels, texture_data_size);
    SDL_UnmapGPUTransferBuffer(gpu_device, texture_transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (upload_command_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to acquire upload command buffer: %s", SDL_GetError());
        SDL_DestroySurface(texture_surface);
        SDL_ReleaseGPUTexture(gpu_device, sprite->texture);
        SDL_ReleaseGPUTransferBuffer(gpu_device, texture_transfer_buffer);
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);
    
    SDL_UploadToGPUTexture
    (
        copy_pass,
        &(SDL_GPUTextureTransferInfo)
        {
            .transfer_buffer = texture_transfer_buffer,
            .offset = 0,
            .pixels_per_row = (Uint32)texture_surface->w,
            .rows_per_layer = (Uint32)texture_surface->h
        },
        &(SDL_GPUTextureRegion)
        {
            .texture = sprite->texture,
            .mip_level = 0,
            .layer = 0,
            .x = 0, .y = 0, .z = 0,
            .w = (Uint32)texture_surface->w,
            .h = (Uint32)texture_surface->h,
            .d = 1
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);

    if (n_mipmap_levels > 1) SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, sprite->texture);

    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    SDL_ReleaseGPUTransferBuffer(gpu_device, texture_transfer_buffer);
    SDL_DestroySurface(texture_surface);

    SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Successfully loaded unanimated sprite: %s", sprite_name);

    return true;
}

bool Sprite_LoadSprites(void)
{
    Array_Initialize(&sprites);
    if (!sprites.arr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize sprites array");
        return false;
    }

    char path[MAXIMUM_URI_LENGTH];
    SDL_snprintf(path, sizeof(path), "%stextures/%s", base_path, "_sprites_list.txt");
    size_t sprites_list_txt_size = 0;
    char* sprites_list_txt = (char*)SDL_LoadFile(path, &sprites_list_txt_size);
    if (!sprites_list_txt)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load sprite list: %s", SDL_GetError());
        return false;
    }

    char* saveptr = NULL;
    char* line = SDL_strtok_r(sprites_list_txt, "\r\n", &saveptr);

    while (line != NULL)
    {
        // Trim leading whitespace
        while (*line == ' ' || *line == '\t') 
            line++;
        
        // Skip empty lines
        if (*line != '\0')
        {
            char* sprite_uri = line;

            char* sprite_height_str; 
            SDL_strtok_r(line, " ", &sprite_height_str);

            Sprite sprite = {0};
            if (!Sprite_Load(sprite_uri, &sprite))
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load sprite: %s", sprite_uri);
                SDL_free(sprites_list_txt);
                return false;
            }
            sprite.height = SDL_atof(sprite_height_str);
            Array_Append(&sprites, sprite);
        }
        
        line = SDL_strtok_r(NULL, "\r\n", &saveptr);
    }

    SDL_free(sprites_list_txt);
    return true;
}