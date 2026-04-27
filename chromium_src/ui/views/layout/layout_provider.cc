// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/views/layout/layout_provider.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#include "base/mac/mac_util.h"
#endif

#include <ui/views/layout/layout_provider.cc>

namespace views {

int LayoutProvider::GetCornerRadiusMetric(ShapeContextTokensOverride id) const {
  switch (id) {
    case ShapeContextTokensOverride::kOmniboxExpandedRadius:
      return 4;
    case ShapeContextTokensOverride::kRoundedCornersBorderRadius:
      // Matches Brave Mac window / content inner rounding.
      return 6;
    case ShapeContextTokensOverride::kRoundedCornersBorderRadiusAtWindowCorner:
#if BUILDFLAG(IS_MAC)
      if (base::mac::MacOSMajorVersion() >= 26) {
        return 16;
      }
#endif
      return 6;
    default:
      break;
  }

  NOTREACHED();
}

}  // namespace views
