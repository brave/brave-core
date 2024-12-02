// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/vector_icons/vector_icons.h"

#define GetSecurityVectorIcon GetSecurityVectorIcon_Chromium

#include "src/components/omnibox/browser/location_bar_model_util.cc"

#undef GetSecurityVectorIcon

namespace location_bar_model {

const gfx::VectorIcon& GetSecurityVectorIcon(
    security_state::SecurityLevel security_level,
    security_state::MaliciousContentStatus malicious_content_status) {
  if (security_level == security_state::SECURE) {
    return kLeoTuneSmallIcon;
  }

  return GetSecurityVectorIcon_Chromium(security_level,
                                        malicious_content_status);
}

}  // namespace location_bar_model
