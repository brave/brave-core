/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/content_settings/content_settings_util.h"

#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

bool IsJavaScriptAllowed() {
  return GlobalState::GetInstance()->ContentSettings().allow_javascript;
}

}  // namespace brave_ads
