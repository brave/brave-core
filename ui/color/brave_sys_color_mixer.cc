// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ui/color/brave_sys_color_mixer.h"

#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"

namespace ui {

// We lightly tweak the dark theme in Grayscale mode (the default theme in
// Brave) to be a bit darker, to not upset dark mode users.
void AddBraveSysColorMixer(ColorProvider* provider,
                           const ColorProviderKey& key) {
  if (key.color_mode != ColorProviderKey::ColorMode::kDark ||
      key.user_color_source != ColorProviderKey::UserColorSource::kGrayscale) {
    return;
  }

  auto& mixer = provider->AddMixer();

  mixer[kColorSysHeader] = {kColorRefNeutral5};
  mixer[kColorSysHeaderInactive] = {kColorRefNeutral5};

  mixer[kColorSysBase] = {kColorRefNeutral15};
  mixer[kColorSysOmniboxContainer] = {kColorRefNeutral5};
}

}  // namespace ui
