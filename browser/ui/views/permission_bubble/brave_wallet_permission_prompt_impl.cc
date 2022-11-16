/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/permission_bubble/brave_wallet_permission_prompt_impl.h"

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "components/permissions/permission_uma_util.h"

BraveWalletPermissionPromptImpl::BraveWalletPermissionPromptImpl(
    Browser* browser,
    content::WebContents* web_contents,
    Delegate* delegate)
    : web_contents_(web_contents),
      delegate_(delegate),
      permission_requested_time_(base::TimeTicks::Now()) {
  DCHECK(delegate_);
  DCHECK(web_contents_);
  ShowBubble();
}

BraveWalletPermissionPromptImpl::~BraveWalletPermissionPromptImpl() {
  brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents_)
      ->CloseBubble();
}

void BraveWalletPermissionPromptImpl::ShowBubble() {
  brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents_)
      ->ShowBubble();
}

bool BraveWalletPermissionPromptImpl::UpdateAnchor() {
  // Returning false will force the caller to recreate the view.
  return false;
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
