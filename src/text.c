#include "text.h"
#include "globals.h"
#include "array.h"

bool Text_Init()
{
    if (!TTF_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_ttf could not initialize! SDL_ttf Error: %s\n", SDL_GetError());
        return false;
    }

    char font_path[MAXIMUM_URI_LENGTH];
    SDL_snprintf(font_path, sizeof(font_path), "%sfonts/ari-w9500-display.ttf", base_path);
    // SDL_snprintf(font_path, sizeof(font_path), "%sfonts/NotoSans-Regular.ttf", base_path);
    
    font = TTF_OpenFont(font_path, 11);

    if (font == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load font! SDL_ttf Error: %s\n", SDL_GetError());
        return false;
    }

    // TTF_SetFontSDF(font, true);
    // TTF_SetFontSizeDPI(font, 64.0f, 96, 96);

    TTF_SetFontHinting(font, TTF_HINTING_LIGHT_SUBPIXEL);
    TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_LEFT);

    textEngine = TTF_CreateGPUTextEngine(gpu_device);
    if (textEngine == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine! SDL_ttf Error: %s\n", SDL_GetError());
        return false;
    }

    text_renderable.ttf_text = TTF_CreateText(textEngine, font, "X", 0);

    if (text_renderable.ttf_text == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create text! SDL_ttf Error: %s\n", SDL_GetError());
        return false;
    }

    // TTF_SetTextWrapWidth(text_renderable.ttf_text, 0);

    // TODO this doesn't seem to do anything...
    // TTF_SetTextColorFloat(text, 1.0f, 1.0f, 1.0f, 1.0f);

    text_vertices_buffer_cpu = SDL_malloc(MAX_TEXT_VERTEX_COUNT * sizeof(Text_Vertex));
    if (text_vertices_buffer_cpu == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to allocate text vertex buffer CPU memory: %s", SDL_GetError());
        return false;
    }
    Uint32 vertex_data_size = (Uint32)(MAX_TEXT_VERTEX_COUNT * sizeof(Text_Vertex));
    text_renderable.vertex_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_data_size
        }
    );
    if (text_renderable.vertex_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create text vertex buffer: %s", SDL_GetError());
        return false;
    }

    text_indices_buffer_cpu = SDL_malloc(MAX_TEXT_INDEX_COUNT * sizeof(int));
    if (text_indices_buffer_cpu == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to allocate text index buffer CPU memory: %s", SDL_GetError());
        return false;
    }
    Uint32 index_data_size = (Uint32)(MAX_TEXT_INDEX_COUNT * sizeof(int));
    text_renderable.index_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_data_size
        }
    );
    if (text_renderable.index_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create index buffer: %s", SDL_GetError());
        return false;
    }

    Uint32 combined_data_size = vertex_data_size + index_data_size;

    text_transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = combined_data_size
        }
    );
    if (text_transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create vertex/index transfer buffer: %s", SDL_GetError());
        return false;
    }

    SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Successfully loaded font and text resources");

    return true;
}

// TODO should call this ASAP in a frame, not at the end with the UI draw calls
bool Text_UpdateAndUpload(const char* new_text)
{
    TTF_SetTextString(text_renderable.ttf_text, new_text, 0);

    text_renderable.vertex_count = 0;
    text_renderable.index_count = 0;

    TTF_GPUAtlasDrawSequence* text_geometry = TTF_GetGPUTextDrawData(text_renderable.ttf_text);
    if (!text_geometry)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create text geometry");
        return false;
    }

    // int text_width, text_height;
    // if (!TTF_GetTextSize(text_renderable.ttf_text, &text_width, &text_height))
    // {
    //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get text size");
    //     return false;
    // }
    // if (text_width == 0 || text_height == 0)
    // {
    //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid text size");
    //     return false;
    // }

    // float aspect_ratio = (float)text_width / (float)text_height;

    text_renderable.draw_sequence = text_geometry;

    while (text_geometry)
    {
        for (size_t i = 0; i < text_geometry->num_vertices; i++)
        {
            // text_vertices_buffer_cpu[text_renderable.vertex_count + i].xy.x = text_geometry->xy[i].x / window_width;
            // text_vertices_buffer_cpu[text_renderable.vertex_count + i].xy.y = text_geometry->xy[i].y / window_height;
            text_vertices_buffer_cpu[text_renderable.vertex_count + i].xy = text_geometry->xy[i];
            text_vertices_buffer_cpu[text_renderable.vertex_count + i].uv = text_geometry->uv[i];
        }

        SDL_memcpy(text_indices_buffer_cpu + text_renderable.index_count, text_geometry->indices, text_geometry->num_indices * sizeof(int));

        text_renderable.vertex_count += text_geometry->num_vertices;
        text_renderable.index_count += text_geometry->num_indices;

        text_geometry = text_geometry->next;
    }

    Text_Vertex* transfer_data_mapped = SDL_MapGPUTransferBuffer(gpu_device, text_transfer_buffer, true);
    if (transfer_data_mapped == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map vertex/index transfer buffer: %s", SDL_GetError());
        return false;
    }

    SDL_memcpy(transfer_data_mapped, text_vertices_buffer_cpu, sizeof(Text_Vertex) * text_renderable.vertex_count);
    SDL_memcpy(transfer_data_mapped + text_renderable.vertex_count, text_indices_buffer_cpu, sizeof(int) * text_renderable.index_count);

    SDL_UnmapGPUTransferBuffer(gpu_device, text_transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (upload_command_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to acquire upload command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_UploadToGPUBuffer
    (
        copy_pass,
        &(SDL_GPUTransferBufferLocation)
        { 
            .transfer_buffer = text_transfer_buffer, 
            .offset = 0 
        },
        &(SDL_GPUBufferRegion)
        { 
            .buffer = text_renderable.vertex_buffer, 
            .offset = 0, 
            .size = text_renderable.vertex_count * sizeof(Text_Vertex) 
        },
        true
    );

    SDL_UploadToGPUBuffer
    (
        copy_pass,
        &(SDL_GPUTransferBufferLocation)
        {
            .transfer_buffer = text_transfer_buffer,
            .offset = text_renderable.vertex_count * sizeof(Text_Vertex)
        },
        &(SDL_GPUBufferRegion)
        {
            .buffer = text_renderable.index_buffer,
            .offset = 0,
            .size = text_renderable.index_count * sizeof(int)
        },
        true
    );

    SDL_EndGPUCopyPass(copy_pass);

    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    return true;
}