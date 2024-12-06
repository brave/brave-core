/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/viz/service/gl/gpu_service_impl.h"

#define InitializeWithHost InitializeWithHost_ChromiumImpl

#include "src/components/viz/service/gl/gpu_service_impl.cc"

#undef InitializeWithHost

#if BUILDFLAG(IS_WIN)
#include <Psapi.h>

#include "base/strings/strcat.h"
#include "base/strings/sys_string_conversions.h"
#endif

namespace viz {

void GpuServiceImpl::InitializeWithHost(
    mojo::PendingRemote<mojom::GpuHost> pending_gpu_host,
    gpu::GpuProcessShmCount use_shader_cache_shm_count,
    scoped_refptr<gl::GLSurface> default_offscreen_surface,
    mojom::GpuServiceCreationParamsPtr creation_params,
#if BUILDFLAG(IS_ANDROID)
    gpu::SyncPointManager* sync_point_manager,
    gpu::SharedImageManager* shared_image_manager,
    gpu::Scheduler* scheduler,
#endif
    base::WaitableEvent* shutdown_event) {
  InitializeWithHost_ChromiumImpl(
      std::move(pending_gpu_host), std::move(use_shader_cache_shm_count),
      std::move(default_offscreen_surface), std::move(creation_params),
#if BUILDFLAG(IS_ANDROID)
      sync_point_manager, shared_image_manager, scheduler,
#endif
      shutdown_event);
#if BUILDFLAG(IS_WIN)
  if (gpu_host_) {
    std::string result;
    {
      CHAR path[MAX_PATH] = {0};
      GetModuleFileNameExA(base::Process::Current().Handle(), nullptr, path,
                           MAX_PATH);
      base::StrAppend(&result,
                      {"GetModuleFileNameExA = ",
                       base::WideToUTF8(base::SysNativeMBToWide(path))});
    }
    {
      WCHAR path[MAX_PATH] = {0};
      GetModuleFileNameExW(base::Process::Current().Handle(), nullptr, path,
                           MAX_PATH);
      base::StrAppend(&result,
                      {"\nGetModuleFileNameExW = ", base::WideToUTF8(path)});
    }
    {
      CHAR path[MAX_PATH] = {0};
      GetModuleFileNameA(nullptr, path, MAX_PATH);
      base::StrAppend(&result,
                      {"\nGetModuleFileNameA = ",
                       base::WideToUTF8(base::SysNativeMBToWide(path))});
    }
    {
      WCHAR path[MAX_PATH] = {0};
      GetModuleFileNameW(nullptr, path, MAX_PATH);
      base::StrAppend(&result,
                      {"\nGetModuleFileNameW = ", base::WideToUTF8(path)});
    }
    gpu_host_->DidGetExecutablePath(result);
  }
#endif
}

}  // namespace viz
