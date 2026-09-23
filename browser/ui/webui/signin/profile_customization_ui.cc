// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/feature_list.h"
#include "brave/grit/brave_generated_resources_webui_strings.h"
#include "brave/ui/webui/custom_profile_image/buildflags/buildflags.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE)
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#endif  // BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE)

namespace brave {

void AddProfileCustomizationData(content::WebUIDataSource* source) {
  bool enabled = false;
#if BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE)
  source->AddLocalizedStrings(webui::kCustomProfileImageStrings);
  enabled = base::FeatureList::IsEnabled(
      custom_profile_image::features::kBraveCustomProfileImage);
#endif  // BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE)

  source->AddBoolean("customProfileImageEnabled", enabled);
}

}  // namespace brave
