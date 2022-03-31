/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/permission_bubble/ethereum_permission_prompt_impl.h"

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "components/permissions/permission_uma_util.h"

EthereumPermissionPromptImpl::EthereumPermissionPromptImpl(
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

EthereumPermissionPromptImpl::~EthereumPermissionPromptImpl() {
  brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents_)
      ->CloseBubble();
}

void EthereumPermissionPromptImpl::ShowBubble() {
  brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents_)
      ->ShowBubble();
}

void EthereumPermissionPromptImpl::UpdateAnchor() {}

permissions::PermissionPrompt::TabSwitchingBehavior
EthereumPermissionPromptImpl::GetTabSwitchingBehavior() {
  return permissions::PermissionPrompt::TabSwitchingBehavior::
      kDestroyPromptButKeepRequestPending;
}

permissions::PermissionPromptDisposition
EthereumPermissionPromptImpl::GetPromptDisposition() const {
  return permissions::PermissionPromptDisposition::ANCHORED_BUBBLE;
}
