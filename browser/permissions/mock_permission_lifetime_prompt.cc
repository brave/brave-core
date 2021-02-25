/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/mock_permission_lifetime_prompt.h"

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

void MockPermissionLifetimePrompt::UpdateAnchorPosition() {}

PermissionPrompt::TabSwitchingBehavior
MockPermissionLifetimePrompt::GetTabSwitchingBehavior() {
  return TabSwitchingBehavior::kDestroyPromptButKeepRequestPending;
}

PermissionPromptDisposition MockPermissionLifetimePrompt::GetPromptDisposition()
    const {
  return PermissionPromptDisposition::ANCHORED_BUBBLE;
}

void MockPermissionLifetimePrompt::ResetFactory() {
  factory_ = nullptr;
}

}  // namespace permissions
