// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/page_action/page_action_controller.h"

#include <chrome/browser/ui/views/page_action/page_action_controller.cc>

namespace page_actions {

PageActionControllerImpl::~PageActionControllerImpl() = default;

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

void PageActionControllerImpl::SetOverrideHeight(actions::ActionId action_id,
                                                 int height) {
  CHECK_GT(height, 0);
  FindPageActionModel(action_id).SetOverrideHeight(PassKey(), height);
}

void PageActionControllerImpl::ClearOverrideHeight(
    actions::ActionId action_id) {
  FindPageActionModel(action_id).SetOverrideHeight(PassKey(), std::nullopt);
}

void PageActionControllerImpl::SetOverrideTriggerableEvent(
    actions::ActionId action_id,
    std::optional<int> event_flags) {
  FindPageActionModel(action_id).SetOverrideTriggerableEvent(PassKey(),
                                                             event_flags);
}

void PageActionControllerImpl::SetOverrideBorder(actions::ActionId action_id,
                                                 const gfx::Insets& insets) {
  FindPageActionModel(action_id).SetOverrideBorder(PassKey(), insets);
}

void PageActionControllerImpl::ClearOverrideBorder(
    actions::ActionId action_id) {
  FindPageActionModel(action_id).SetOverrideBorder(PassKey(), std::nullopt);
}

std::unique_ptr<PageActionModelInterface> PageActionControllerImpl::CreateModel(
    actions::ActionId action_id,
    bool is_ephemeral) {
  if (page_action_model_factory_ != nullptr) {
    return chromium_impl::PageActionControllerImpl::CreateModel(action_id,
                                                                is_ephemeral);
  }

  return std::make_unique<PageActionModel>(is_ephemeral);
}

}  // namespace page_actions
