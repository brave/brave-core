/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_GPU_GPU_PROCESS_HOST_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_GPU_GPU_PROCESS_HOST_H_

#include "build/build_config.h"
#include "components/viz/host/gpu_host_impl.h"
#include "services/viz/privileged/mojom/gl/gpu_host.mojom.h"

#if BUILDFLAG(IS_WIN)
// |executable_path_| used as storage for GPU process executable path.
#define DidFailInitialize                                 \
  DidGetExecutablePath(const std::string& path) override; \
                                                          \
 public:                                                  \
  const std::string& executable_path() const {            \
    return executable_path_;                              \
  }                                                       \
                                                          \
 private:                                                 \
  std::string executable_path_;                           \
  void DidFailInitialize
#endif

#include "src/content/browser/gpu/gpu_process_host.h"  // IWYU pragma: export

#undef DidFailInitialize

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_GPU_GPU_PROCESS_HOST_H_
