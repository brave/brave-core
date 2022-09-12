/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "build/build_config.h"
#include "media/base/key_system_info.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(IS_ANDROID) && !BUILDFLAG(ENABLE_WIDEVINE)
namespace cdm {
// Fix upstream build failure with widevine disabled on android.
// chrome_key_systems.cc calls cdm::AddAndroidWidevine() regardless of
// ENABLE_WIDEVINE build flag.
// But, it's only declared in that build flag. See android_key_systems.h.
void AddAndroidWidevine(
    std::vector<std::unique_ptr<media::KeySystemProperties>>*
        concrete_key_systems) {
}
}  // namespace cdm
#endif

#include "src/chrome/renderer/media/chrome_key_systems.cc"
