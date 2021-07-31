/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_ETHEREUM_PERMISSION_PROMPT_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_ETHEREUM_PERMISSION_PROMPT_IMPL_H_

#include "components/permissions/permission_prompt.h"

class Browser;

class EthereumPermissionPromptImpl : public permissions::PermissionPrompt {
 public:
  EthereumPermissionPromptImpl(Browser* browser,
                               content::WebContents* web_contents,
                               Delegate* delegate);
  ~EthereumPermissionPromptImpl() override;

  EthereumPermissionPromptImpl(const EthereumPermissionPromptImpl&) = delete;
  EthereumPermissionPromptImpl& operator=(const EthereumPermissionPromptImpl&) =
      delete;

  // permissions::PermissionPrompt:
  void UpdateAnchor() override;
  TabSwitchingBehavior GetTabSwitchingBehavior() override;
  permissions::PermissionPromptDisposition GetPromptDisposition()
      const override;

 private:
  void ShowBubble();

  content::WebContents* web_contents_;
  permissions::PermissionPrompt::Delegate* const delegate_;
  base::TimeTicks permission_requested_time_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_ETHEREUM_PERMISSION_PROMPT_IMPL_H_
