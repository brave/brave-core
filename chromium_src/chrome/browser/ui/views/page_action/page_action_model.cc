// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <chrome/browser/ui/views/page_action/page_action_model.cc>

namespace page_actions {

void PageActionModel::SetAlwaysShowLabel(base::PassKey<PageActionController>,
                                         bool always_show) {
  if (always_show_label_ == always_show) {
    return;
  }
  always_show_label_ = always_show;
  NotifyChange(Property::kAlwaysShowLabel);
}

bool PageActionModel::GetAlwaysShowLabel() const {
  return always_show_label_;
}

void PageActionModel::SetOverrideChipColors(
    base::PassKey<PageActionController>,
    std::optional<SkColor> override_background_color,
    std::optional<SkColor> override_foreground_color) {
  if (override_background_color_ == override_background_color &&
      override_foreground_color_ == override_foreground_color) {
    return;
  }
  override_background_color_ = override_background_color;
  override_foreground_color_ = override_foreground_color;
  NotifyChange(Property::kOverrideChipColors);
}

std::optional<SkColor> PageActionModel::GetOverrideBackgroundColor() const {
  return override_background_color_;
}

std::optional<SkColor> PageActionModel::GetOverrideForegroundColor() const {
  return override_foreground_color_;
}

}  // namespace page_actions
