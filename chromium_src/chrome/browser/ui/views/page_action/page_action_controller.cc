// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <chrome/browser/ui/views/page_action/page_action_controller.cc>

namespace page_actions {

void PageActionControllerImpl::SetAlwaysShowLabel(actions::ActionId action_id,
                                                  bool always_show) {
  FindPageActionModel(action_id).SetAlwaysShowLabel(PassKey(), always_show);
}

void PageActionControllerImpl::OverrideChipColors(
    actions::ActionId action_id,
    std::optional<SkColor> override_background_color,
    std::optional<SkColor> override_foreground_color) {
  FindPageActionModel(action_id).SetOverrideChipColors(
      PassKey(), override_background_color, override_foreground_color);
}

void PageActionControllerImpl::ClearOverrideChipColors(
    actions::ActionId action_id) {
  FindPageActionModel(action_id).SetOverrideChipColors(
      PassKey(), /*override_background_color=*/std::nullopt,
      /*override_foreground_color=*/std::nullopt);
}

}  // namespace page_actions
