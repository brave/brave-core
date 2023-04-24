/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_flag_util.h"

#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

bool DidOverrideCommandLine() {
  return GlobalState::GetInstance()->Flags().did_override_from_command_line;
}

}  // namespace brave_ads
