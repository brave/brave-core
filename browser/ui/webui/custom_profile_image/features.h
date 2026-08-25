// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_CUSTOM_PROFILE_IMAGE_FEATURES_H_
#define BRAVE_BROWSER_UI_WEBUI_CUSTOM_PROFILE_IMAGE_FEATURES_H_

#include "base/feature.h"
#include "brave/browser/ui/custom_profile_image_buildflags.h"

static_assert(BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE));

namespace custom_profile_image::features {

BASE_DECLARE_FEATURE(kBraveCustomProfileImage);

}  // namespace custom_profile_image::features

#endif  // BRAVE_BROWSER_UI_WEBUI_CUSTOM_PROFILE_IMAGE_FEATURES_H_
