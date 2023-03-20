/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VIZ_HOST_GPU_HOST_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VIZ_HOST_GPU_HOST_IMPL_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#define GetGPUInfo                                              \
  NotUsed();                                                    \
  virtual void DidGetExecutablePath(const std::string& path) {} \
  virtual gpu::GPUInfo GetGPUInfo

#define GetShaderPrefixKey                                     \
  NotUsed() {                                                  \
    return {};                                                 \
  }                                                            \
  void DidGetExecutablePath(const std::string& path) override; \
  std::string GetShaderPrefixKey
#endif

#include "src/components/viz/host/gpu_host_impl.h"  // IWYU pragma: export

#undef GetGPUInfo
#undef GetShaderPrefixKey

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VIZ_HOST_GPU_HOST_IMPL_H_
