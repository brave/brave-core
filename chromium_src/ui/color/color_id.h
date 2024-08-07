// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_COLOR_COLOR_ID_H_
#define BRAVE_CHROMIUM_SRC_UI_COLOR_COLOR_ID_H_

// These are Material colors which Chromium doesn't add to its color mixer but
// which we need for Nala. We have a mixer in
// brave/ui/color/missing_color_ref_mixer.h for adding these missing Material
// colors.
// This isn't undefined because other files use the |COLOR_ID| macro which
// depends on this being defined.
#define BRAVE_MATERIAL_MISSING_COLOR_IDS \
  E_CPONLY(kColorRefNeutral5)            \
  E_CPONLY(kColorRefNeutral35)           \
  E_CPONLY(kColorRefNeutralVariant5)     \
  E_CPONLY(kColorRefNeutralVariant25)    \
  E_CPONLY(kColorRefNeutralVariant35)    \
  E_CPONLY(kColorRefNeutralVariant98)    \
  E_CPONLY(kColorRefPrimary5)            \
  E_CPONLY(kColorRefPrimary15)           \
  E_CPONLY(kColorRefPrimary35)           \
  E_CPONLY(kColorRefPrimary98)           \
  E_CPONLY(kColorRefSecondary5)          \
  E_CPONLY(kColorRefSecondary98)         \
  E_CPONLY(kColorRefTertiary5)           \
  E_CPONLY(kColorRefTertiary15)          \
  E_CPONLY(kColorRefTertiary25)          \
  E_CPONLY(kColorRefTertiary35)          \
  E_CPONLY(kColorRefTertiary98)

#include "src/ui/color/color_id.h"  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_UI_COLOR_COLOR_ID_H_
