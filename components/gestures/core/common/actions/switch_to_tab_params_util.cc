// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/switch_to_tab_params_util.h"

#include "base/values.h"

namespace gestures {

SwitchToTabParamsInfo ParseSwitchToTabParams(
    const base::DictValue& action_dict) {
  SwitchToTabParamsInfo switch_to_tab_params;
  if (const std::string* pattern =
          action_dict.FindString("switch_to_tab_pattern")) {
    switch_to_tab_params.pattern = *pattern;
  }
  return switch_to_tab_params;
}

}  // namespace gestures
