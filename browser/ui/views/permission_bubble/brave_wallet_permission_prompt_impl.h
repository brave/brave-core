/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_BRAVE_WALLET_PERMISSION_PROMPT_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_BRAVE_WALLET_PERMISSION_PROMPT_IMPL_H_

#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "components/permissions/permission_prompt.h"

class Browser;

class BraveWalletPermissionPromptImpl : public permissions::PermissionPrompt {
 public:
  BraveWalletPermissionPromptImpl(Browser* browser,
                                  content::WebContents* web_contents,
                                  Delegate& delegate);
  ~BraveWalletPermissionPromptImpl() override;

  BraveWalletPermissionPromptImpl(const BraveWalletPermissionPromptImpl&) =
      delete;
  BraveWalletPermissionPromptImpl& operator=(
      const BraveWalletPermissionPromptImpl&) = delete;

  // permissions::PermissionPrompt:
  bool UpdateAnchor() override;
  TabSwitchingBehavior GetTabSwitchingBehavior() override;
  permissions::PermissionPromptDisposition GetPromptDisposition()
      const override;
  bool IsAskPrompt() const override;
  std::optional<gfx::Rect> GetViewBoundsInScreen() const override;
  bool ShouldFinalizeRequestAfterDecided() const override;
  std::vector<permissions::ElementAnchoredBubbleVariant> GetPromptVariants()
      const override;
  std::optional<permissions::feature_params::PermissionElementPromptPosition>
  GetPromptPosition() const override;

 private:
  void ShowBubble();

  const raw_ptr<content::WebContents> web_contents_ = nullptr;
  raw_ref<permissions::PermissionPrompt::Delegate> delegate_;
  base::TimeTicks permission_requested_time_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_BRAVE_WALLET_PERMISSION_PROMPT_IMPL_H_
