/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <vector>

#include "media/base/key_system_info.h"
#include "third_party/widevine/cdm/buildflags.h"

// In components/cdm/renderer/android_key_systems.h the declaration of
// AddAndroidWidevine is guarded by ENABLE_WIDEVINE, but in
// content_renderer_client_impl below it is used without a guard. Instead of
// patching we can add an empty definition of the same function here for when
// ENABLE_WIDEVINE is not true.

#if !BUILDFLAG(ENABLE_WIDEVINE)
namespace cdm {

void AddAndroidWidevine(
    std::vector<std::unique_ptr<media::KeySystemProperties>>*
        concrete_key_systems) {}

}  // namespace cdm
#endif  // BUILDFLAG(ENABLE_WIDEVINE)

#include "src/weblayer/renderer/content_renderer_client_impl.cc"
