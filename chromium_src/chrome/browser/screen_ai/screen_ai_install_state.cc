/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/screen_ai/screen_ai_install_state.h"

#define ShouldInstall ShouldInstall_ChromiumImpl
#include "src/chrome/browser/screen_ai/screen_ai_install_state.cc"
#undef ShouldInstall

namespace screen_ai {
bool ScreenAIInstallState::ShouldInstall(PrefService* local_state) {
  return false;
}
}  // namespace screen_ai
