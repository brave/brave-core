/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_MEDIA_MOJO_SERVICES_GPU_MOJO_MEDIA_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_MEDIA_MOJO_SERVICES_GPU_MOJO_MEDIA_CLIENT_H_

// Include header of the base class to avoid being affected by the #define below
#include "media/mojo/services/mojo_media_client.h"

// Inject a method that we can use to get around a compilation error of
// gpu_memory_buffer_factory_ being unused. The error happens because the member
// is guarded by BUILDFLAG(USE_CHROMEOS_MEDIA_ACCELERATION), which is true for
// us, since we have use_vaapi set to true for our builds (see the definitions
// of the flags in media/gpu/BUILD.gn). But in .cc gpu_memory_buffer_factory_ is
// only used under OS_CHROMESOS.
#define CreateCdmFactory \
  UnusedMethod();        \
  std::unique_ptr<CdmFactory> CreateCdmFactory
#include "../../../../../media/mojo/services/gpu_mojo_media_client.h"
#undef CreateCdmFactory

#endif  // BRAVE_CHROMIUM_SRC_MEDIA_MOJO_SERVICES_GPU_MOJO_MEDIA_CLIENT_H_
