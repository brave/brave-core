/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/permission_bubble/brave_wallet_permission_prompt_impl.h"

#include <optional>

#include "base/check.h"
#include "base/check_deref.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "components/permissions/permission_uma_util.h"
#include "components/tabs/public/tab_interface.h"

BraveWalletPermissionPromptImpl::BraveWalletPermissionPromptImpl(
    content::WebContents* web_contents,
    Delegate& delegate)
    : web_contents_(web_contents),
      delegate_(delegate),
      permission_requested_time_(base::TimeTicks::Now()) {
  DCHECK(web_contents_);
  ShowBubble();
}

BraveWalletPermissionPromptImpl::~BraveWalletPermissionPromptImpl() {
  // The tab helper is already gone, and closed its bubble, when the prompt is
  // destroyed with the tab.
  if (auto* tab_helper = brave_wallet::BraveWalletTabHelper::From(
          tabs::TabInterface::MaybeGetFromContents(web_contents_))) {
    tab_helper->CloseBubble();
  }
}

void BraveWalletPermissionPromptImpl::ShowBubble() {
  CHECK_DEREF(brave_wallet::BraveWalletTabHelper::From(
                  tabs::TabInterface::MaybeGetFromContents(web_contents_)))
      .ShowBubble();
}

bool BraveWalletPermissionPromptImpl::UpdateAnchor() {
  // Don't recreate the view for every BrowserView::Layout() which would cause
  // BraveWalletPermissionPromptImpl being destoryed which leads to bubble
  // dismissed unintentionally.
  return true;
}

permissions::PermissionPrompt::TabSwitchingBehavior
BraveWalletPermissionPromptImpl::GetTabSwitchingBehavior() {
  return permissions::PermissionPrompt::TabSwitchingBehavior::
      kDestroyPromptButKeepRequestPending;
}

permissions::PermissionPromptDisposition
BraveWalletPermissionPromptImpl::GetPromptDisposition() const {
  return permissions::PermissionPromptDisposition::ANCHORED_BUBBLE;
}

bool BraveWalletPermissionPromptImpl::IsAskPrompt() const {
  return true;
}

std::optional<gfx::Rect>
BraveWalletPermissionPromptImpl::GetViewBoundsInScreen() const {
  return std::nullopt;
}

std::vector<permissions::ElementAnchoredBubbleVariant>
BraveWalletPermissionPromptImpl::GetPromptVariants() const {
  return {};
}

std::optional<permissions::feature_params::PermissionElementPromptPosition>
BraveWalletPermissionPromptImpl::GetPromptPosition() const {
  return std::nullopt;
}
