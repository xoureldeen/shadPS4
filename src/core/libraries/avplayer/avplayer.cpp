// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "avplayer.h"

#include "avplayer_impl.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/libs.h"

#include <mutex>

namespace Libraries::AvPlayer {

using namespace Kernel;

s32 PS4_SYSV_ABI sceAvPlayerAddSource(SceAvPlayerHandle handle, const char* filename) {
    LOG_TRACE(Lib_AvPlayer, "filename = {}", filename);
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->AddSource(filename);
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

s32 PS4_SYSV_ABI sceAvPlayerAddSourceEx(SceAvPlayerHandle handle, SceAvPlayerUriType uriType,
                                        SceAvPlayerSourceDetails* sourceDetails) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAvPlayerChangeStream() {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerClose(SceAvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    delete handle;
    return ORBIS_OK;
}

u64 PS4_SYSV_ABI sceAvPlayerCurrentTime(SceAvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->CurrentTime();
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

s32 PS4_SYSV_ABI sceAvPlayerDisableStream(SceAvPlayerHandle handle, u32 stream_id) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerEnableStream(SceAvPlayerHandle handle, u32 stream_id) {
    LOG_TRACE(Lib_AvPlayer, "stream_id = {}", stream_id);
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->EnableStream(stream_id);
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

bool PS4_SYSV_ABI sceAvPlayerGetAudioData(SceAvPlayerHandle handle, SceAvPlayerFrameInfo* p_info) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || p_info == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->GetAudioData(*p_info);
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

s32 PS4_SYSV_ABI sceAvPlayerGetStreamInfo(SceAvPlayerHandle handle, u32 stream_id,
                                          SceAvPlayerStreamInfo* p_info) {
    LOG_TRACE(Lib_AvPlayer, "stream_id = {}", stream_id);
    if (handle == nullptr || p_info == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->GetStreamInfo(stream_id, *p_info);
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

bool PS4_SYSV_ABI sceAvPlayerGetVideoData(SceAvPlayerHandle handle,
                                          SceAvPlayerFrameInfo* video_info) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || video_info == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->GetVideoData(*video_info);
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

bool PS4_SYSV_ABI sceAvPlayerGetVideoDataEx(SceAvPlayerHandle handle,
                                            SceAvPlayerFrameInfoEx* video_info) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || video_info == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->GetVideoData(*video_info);
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

constexpr u32 GetPriority(u32 base, u32 offset) {
    // (27D <= base_priority <= 2FC) + offset <= 2FF
    return std::min(std::min(std::max(637u, base), 764u) + offset, 767u);
}

SceAvPlayerHandle PS4_SYSV_ABI sceAvPlayerInit(SceAvPlayerInitData* data) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (data == nullptr) {
        return nullptr;
    }

    if (data->memory_replacement.allocate == nullptr ||
        data->memory_replacement.allocate_texture == nullptr ||
        data->memory_replacement.deallocate == nullptr ||
        data->memory_replacement.deallocate_texture == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "All allocators are required for AVPlayer Initialisation.");
        return nullptr;
    }

    ThreadPriorities priorities{};
    const u32 base_priority = data->base_priority != 0 ? data->base_priority : 700;
    priorities.video_decoder_priority = GetPriority(base_priority, 5);
    priorities.audio_decoder_priority = GetPriority(base_priority, 6);
    priorities.demuxer_priority = GetPriority(base_priority, 9);
    priorities.controller_priority = GetPriority(base_priority, 2);
    // priorities.http_streaming_priority = GetPriority(base_priority, 10);
    // priorities.file_streaming_priority = GetPriority(priorities.http_streaming_priority, 15);
    // priorities.maxPriority = priorities.http_streaming_priority;

    const auto player = new AvPlayer();
    player->Init(*data, priorities);
    return player;
}

s32 PS4_SYSV_ABI sceAvPlayerInitEx(const SceAvPlayerInitDataEx* p_data,
                                   SceAvPlayerHandle* p_player) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (p_data == nullptr || p_player == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }

    if (p_data->memory_replacement.allocate == nullptr ||
        p_data->memory_replacement.allocate_texture == nullptr ||
        p_data->memory_replacement.deallocate == nullptr ||
        p_data->memory_replacement.deallocate_texture == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "All allocators are required for AVPlayer Initialisation.");
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }

    SceAvPlayerInitData data = {};
    data.memory_replacement = p_data->memory_replacement;
    data.file_replacement = p_data->file_replacement;
    data.event_replacement = p_data->event_replacement;
    data.default_language = p_data->default_language;
    data.num_output_video_framebuffers = p_data->num_output_video_framebuffers;
    data.auto_start = p_data->auto_start;

    ThreadPriorities priorities{};
    s32 base_priority = 0;
    const auto res = scePthreadGetprio(scePthreadSelf(), &base_priority);
    if (res != 0 || base_priority == 0) {
        base_priority = 700;
    }

    if (p_data->video_decoder_priority != 0) {
        priorities.video_decoder_priority = p_data->video_decoder_priority;
    } else {
        priorities.video_decoder_priority = GetPriority(base_priority, 5);
    }
    priorities.video_decoder_affinity = p_data->video_decoder_affinity;

    if (p_data->audio_decoder_priority != 0) {
        priorities.audio_decoder_priority = p_data->audio_decoder_priority;
    } else {
        priorities.audio_decoder_priority = GetPriority(base_priority, 6);
    }
    priorities.audio_decoder_affinity = p_data->audio_decoder_affinity;

    if (p_data->controller_priority != 0) {
        priorities.controller_priority = p_data->controller_priority;
    } else {
        priorities.controller_priority = GetPriority(base_priority, 2);
    }
    priorities.controller_affinity = p_data->controller_affinity;

    if (p_data->demuxer_priority != 0) {
        priorities.demuxer_priority = p_data->demuxer_priority;
    } else {
        priorities.demuxer_priority = GetPriority(base_priority, 9);
    }
    priorities.demuxer_affinity = p_data->demuxer_affinity;

    // if (p_data->http_streaming_priority != 0) {
    //     priorities.http_streaming_priority = p_data->http_streaming_priority;
    // } else {
    //     priorities.http_streaming_priority = GetPriority(base_priority, 10);
    // }
    // priorities.http_streaming_affinity = p_data->http_streaming_affinity;

    // if (p_data->file_streaming_priority != 0) {
    //     priorities.file_streaming_priority = p_data->file_streaming_priority;
    // } else {
    //     priorities.file_streaming_priority = GetPriority(base_priority, 15);
    // }
    // priorities.http_streaming_affinity = p_data->http_streaming_affinity;

    const auto player = new AvPlayer();
    player->Init(data, priorities);
    *p_player = player;
    return ORBIS_OK;
}

bool PS4_SYSV_ABI sceAvPlayerIsActive(SceAvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        LOG_TRACE(Lib_AvPlayer, "returning ORBIS_AVPLAYER_ERROR_INVALID_PARAMS");
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->IsActive();
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

s32 PS4_SYSV_ABI sceAvPlayerJumpToTime(SceAvPlayerHandle handle, uint64_t jump_time_msec) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerPause(SceAvPlayerHandle handle) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerPostInit(SceAvPlayerHandle handle, SceAvPlayerPostInitData* data) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || data == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->PostInit(*data);
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

s32 PS4_SYSV_ABI sceAvPlayerPrintf(const char* format, ...) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerResume(SceAvPlayerHandle handle) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerSetAvSyncMode(SceAvPlayerHandle handle,
                                          SceAvPlayerAvSyncMode sync_mode) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerSetLogCallback(SceAvPlayerLogCallback logCb, void* user_data) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerSetLooping(SceAvPlayerHandle handle, bool loop_flag) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerSetTrickSpeed(SceAvPlayerHandle handle, s32 trick_speed) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerStart(SceAvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->Start();
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

s32 PS4_SYSV_ABI sceAvPlayerStop(SceAvPlayerHandle handle) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->Stop();
}

s32 PS4_SYSV_ABI sceAvPlayerStreamCount(SceAvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto res = handle->GetStreamCount();
    LOG_TRACE(Lib_AvPlayer, "returning {}", res);
    return res;
}

s32 PS4_SYSV_ABI sceAvPlayerVprintf(const char* format, va_list args) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceAvPlayer(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("KMcEa+rHsIo", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerAddSource);
    LIB_FUNCTION("x8uvuFOPZhU", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerAddSourceEx);
    LIB_FUNCTION("buMCiJftcfw", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerChangeStream);
    LIB_FUNCTION("NkJwDzKmIlw", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerClose);
    LIB_FUNCTION("wwM99gjFf1Y", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerCurrentTime);
    LIB_FUNCTION("BOVKAzRmuTQ", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerDisableStream);
    LIB_FUNCTION("ODJK2sn9w4A", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerEnableStream);
    LIB_FUNCTION("Wnp1OVcrZgk", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerGetAudioData);
    LIB_FUNCTION("d8FcbzfAdQw", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerGetStreamInfo);
    LIB_FUNCTION("o3+RWnHViSg", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerGetVideoData);
    LIB_FUNCTION("JdksQu8pNdQ", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerGetVideoDataEx);
    LIB_FUNCTION("aS66RI0gGgo", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerInit);
    LIB_FUNCTION("o9eWRkSL+M4", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerInitEx);
    LIB_FUNCTION("UbQoYawOsfY", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerIsActive);
    LIB_FUNCTION("XC9wM+xULz8", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerJumpToTime);
    LIB_FUNCTION("9y5v+fGN4Wk", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerPause);
    LIB_FUNCTION("HD1YKVU26-M", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerPostInit);
    LIB_FUNCTION("agig-iDRrTE", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerPrintf);
    LIB_FUNCTION("w5moABNwnRY", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerResume);
    LIB_FUNCTION("k-q+xOxdc3E", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerSetAvSyncMode);
    LIB_FUNCTION("eBTreZ84JFY", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerSetLogCallback);
    LIB_FUNCTION("OVths0xGfho", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerSetLooping);
    LIB_FUNCTION("av8Z++94rs0", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerSetTrickSpeed);
    LIB_FUNCTION("ET4Gr-Uu07s", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerStart);
    LIB_FUNCTION("ZC17w3vB5Lo", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerStop);
    LIB_FUNCTION("hdTyRzCXQeQ", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0,
                 sceAvPlayerStreamCount);
    LIB_FUNCTION("yN7Jhuv8g24", "libSceAvPlayer", 1, "libSceAvPlayer", 1, 0, sceAvPlayerVprintf);
};

} // namespace Libraries::AvPlayer