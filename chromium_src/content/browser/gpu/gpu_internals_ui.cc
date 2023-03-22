/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#define BRAVE_GPU_MESSAGE_HANDLER_ON_REQUEST_CLIENT_INFO  \
  if (auto* host = GpuProcessHost::Get()) {               \
    dict.Set("executable_path", host->executable_path()); \
  }
#else
#define BRAVE_GPU_MESSAGE_HANDLER_ON_REQUEST_CLIENT_INFO
#endif

#include "src/content/browser/gpu/gpu_internals_ui.cc"

#undef BRAVE_GPU_MESSAGE_HANDLER_ON_REQUEST_CLIENT_INFO
