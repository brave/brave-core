/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/whats_new/whats_new_util.h"

#include "brave/browser/ui/whats_new/whats_new_util.h"

#define ShouldShowForState ShouldShowForState_UnUsed
#include "src/chrome/browser/ui/webui/whats_new/whats_new_util.cc"
#undef ShouldShowForState

namespace whats_new {

bool ShouldShowForState(PrefService* local_state,
                        bool promotional_tabs_enabled) {
  return ShouldShowBraveWhatsNewForState(local_state);
}

}  // namespace whats_new
