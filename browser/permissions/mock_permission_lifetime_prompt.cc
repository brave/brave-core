/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/mock_permission_lifetime_prompt.h"

#include <optional>

#include "brave/browser/permissions/mock_permission_lifetime_prompt_factory.h"

namespace permissions {

MockPermissionLifetimePrompt::MockPermissionLifetimePrompt(
    MockPermissionLifetimePromptFactory* factory,
    Delegate* delegate)
    : factory_(factory), delegate_(delegate) {}

MockPermissionLifetimePrompt::~MockPermissionLifetimePrompt() {
  if (factory_)
    factory_->HideView(this);
}

bool MockPermissionLifetimePrompt::UpdateAnchor() {
  return false;
}

PermissionPrompt::TabSwitchingBehavior
MockPermissionLifetimePrompt::GetTabSwitchingBehavior() {
  return TabSwitchingBehavior::kDestroyPromptButKeepRequestPending;
}

PermissionPromptDisposition MockPermissionLifetimePrompt::GetPromptDisposition()
    const {
  return PermissionPromptDisposition::ANCHORED_BUBBLE;
}

bool MockPermissionLifetimePrompt::IsAskPrompt() const {
  return true;
}

void MockPermissionLifetimePrompt::ResetFactory() {
  factory_ = nullptr;
}

std::optional<gfx::Rect> MockPermissionLifetimePrompt::GetViewBoundsInScreen()
    const {
  return std::nullopt;
}

bool MockPermissionLifetimePrompt::ShouldFinalizeRequestAfterDecided() const {
  return true;
}

std::vector<permissions::ElementAnchoredBubbleVariant>
MockPermissionLifetimePrompt::GetPromptVariants() const {
  return {};
}

std::optional<feature_params::PermissionElementPromptPosition>
MockPermissionLifetimePrompt::GetPromptPosition() const {
  return std::nullopt;
}

}  // namespace permissions
