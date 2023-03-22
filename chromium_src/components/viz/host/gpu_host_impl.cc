/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/viz/host/gpu_host_impl.h"

#include "src/components/viz/host/gpu_host_impl.cc"

namespace viz {

#if BUILDFLAG(IS_WIN)
gpu::GPUInfo GpuHostImpl::Delegate::NotUsed() {
  return {};
}

void GpuHostImpl::DidGetExecutablePath(const std::string& path) {
  delegate_->DidGetExecutablePath(path);
}
#endif

}  // namespace viz
