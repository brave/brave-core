// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/darker_theme/darker_theme_color_transform_factory.h"

#include "base/functional/bind.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_transform.h"
#include "ui/gfx/color_utils.h"

#include "base/logging.h"

namespace darker_theme {

namespace {

SkColor DarkerColorGenerator(const ui::ColorProviderKey& key,
                             int reference_color_id,
                             SkColor input,
                             const ui::ColorMixer& mixer) {
  if (!key.user_color.has_value()) {
    return mixer.GetResultColor(reference_color_id);
  }

  // Takes lightneess from reference color and applies it to input color.
  SkColor reference_color = mixer.GetResultColor(reference_color_id);
  color_utils::HSL reference_hsl;
  color_utils::SkColorToHSL(reference_color, &reference_hsl);

  color_utils::HSL input_hsl;
  color_utils::SkColorToHSL(input, &input_hsl);
  input_hsl.l = reference_hsl.l;  // keep only lightness of reference color

  return color_utils::HSLToSkColor(input_hsl, 255);
}

}  // namespace

ui::ColorTransform ApplyDarknessFromColor(const ui::ColorProviderKey& key,
                                          int reference_color_id) {
  return base::BindRepeating(DarkerColorGenerator, key, reference_color_id);
}

}  // namespace darker_theme
