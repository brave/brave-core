// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "build/build_config.h"
#include "components/omnibox/browser/buildflags.h"

#if (!BUILDFLAG(IS_ANDROID) || BUILDFLAG(ENABLE_VR)) && !BUILDFLAG(IS_IOS)
#include "brave/components/vector_icons/vector_icons.h"
#endif

#define GetSecurityVectorIcon GetSecurityVectorIcon_Chromium

#include <components/omnibox/browser/location_bar_model_util.cc>

#undef GetSecurityVectorIcon

namespace location_bar_model {

const gfx::VectorIcon& GetSecurityVectorIcon(
    security_state::SecurityLevel security_level,
    security_state::VisibleSecurityState* visible_security_state) {
#if (!BUILDFLAG(IS_ANDROID) || BUILDFLAG(ENABLE_VR)) && !BUILDFLAG(IS_IOS)
  if (security_level == security_state::SECURE) {
    return kLeoTuneSmallIcon;
  }
#endif
  return GetSecurityVectorIcon_Chromium(security_level, visible_security_state);
}

}  // namespace location_bar_model
