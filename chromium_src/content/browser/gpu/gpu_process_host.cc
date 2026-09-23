/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/gpu/gpu_process_host.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "sandbox/policy/features.h"
#endif  // BUILDFLAG(IS_WIN)

#include <content/browser/gpu/gpu_process_host.cc>

#if BUILDFLAG(IS_WIN)

namespace content {

void GpuProcessHost::DidGetExecutablePath(const std::string& path) {
  executable_path_ = path;
}

}  // namespace content

#endif  // BUILDFLAG(IS_WIN)
