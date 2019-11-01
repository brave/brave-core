/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../media/mojo/services/gpu_mojo_media_client.cc"

namespace media {

// See .h file for why this is needed.
std::unique_ptr<CdmFactory> GpuMojoMediaClient::UnusedMethod() {
#if BUILDFLAG(USE_CHROMEOS_MEDIA_ACCELERATION)
  (void)gpu_memory_buffer_factory_;
#endif  // BUILDFLAG(USE_CHROMEOS_MEDIA_ACCELERATION)
  return nullptr;
}

}  // namespace media
