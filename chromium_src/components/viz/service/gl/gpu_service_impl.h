/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VIZ_SERVICE_GL_GPU_SERVICE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VIZ_SERVICE_GL_GPU_SERVICE_IMPL_H_

// https://github.com/brave/brave-core/pull/17671
// To check that 'ModuleFilenamePatch' feature works, we provide a result of
// call to the interested WINAPI functions.
#define InitializeWithHost                                             \
  InitializeWithHost_ChromiumImpl(                                     \
      mojo::PendingRemote<mojom::GpuHost> pending_gpu_host,            \
      gpu::GpuProcessShmCount use_shader_cache_shm_count,              \
      scoped_refptr<gl::GLSurface> default_offscreen_surface,          \
      gpu::SyncPointManager* sync_point_manager,                       \
      gpu::SharedImageManager* shared_image_manager,                   \
      gpu::Scheduler* scheduler, base::WaitableEvent* shutdown_event); \
                                                                       \
  void InitializeWithHost

#include "src/components/viz/service/gl/gpu_service_impl.h"  // IWYU pragma: export

#undef InitializeWithHost

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VIZ_SERVICE_GL_GPU_SERVICE_IMPL_H_
