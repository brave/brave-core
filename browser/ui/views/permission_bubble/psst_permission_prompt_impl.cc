/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/permission_bubble/psst_permission_prompt_impl.h"

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

PsstPermissionPromptImpl::PsstPermissionPromptImpl(
    content::WebContents* web_contents,
    Delegate* delegate)
    : web_contents_(web_contents), delegate_(delegate) {
  // Create your custom dialog/UI here
  ShowCustomDialog();
}

PsstPermissionPromptImpl::~PsstPermissionPromptImpl() {
  // Close UI
}

bool PsstPermissionPromptImpl::UpdateAnchor() {
  // Update dialog position if needed
  return true;
}

PsstPermissionPromptImpl::TabSwitchingBehavior
PsstPermissionPromptImpl::GetTabSwitchingBehavior() {
  return TabSwitchingBehavior::kKeepPromptAlive;
}

void PsstPermissionPromptImpl::ShowCustomDialog() {
// Get PSST observer and trigger ShowBubble
  if (auto* psst_observer = 
      psst::PsstTabWebContentsObserver::FromWebContents(web_contents_)) {
    psst_observer->ShowBubble(delegate_);
  } else {
    // Fallback: deny permission if no PSST observer
    delegate_->Deny();
    delegate_->Dismiss();
  }
}

permissions::PermissionPromptDisposition
PsstPermissionPromptImpl::GetPromptDisposition() const {
  return permissions::PermissionPromptDisposition::ANCHORED_BUBBLE;
}

bool PsstPermissionPromptImpl::IsAskPrompt() const {
  return true;
}

std::optional<gfx::Rect>
PsstPermissionPromptImpl::GetViewBoundsInScreen() const {
  return std::nullopt;
}

bool PsstPermissionPromptImpl::ShouldFinalizeRequestAfterDecided() const {
  return true;
}

std::vector<permissions::ElementAnchoredBubbleVariant>
PsstPermissionPromptImpl::GetPromptVariants() const {
  return {};
}

std::optional<permissions::feature_params::PermissionElementPromptPosition>
PsstPermissionPromptImpl::GetPromptPosition() const {
  return std::nullopt;
}
