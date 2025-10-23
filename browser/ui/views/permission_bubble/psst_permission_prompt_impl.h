/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_PSST_PERMISSION_PROMPT_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_PSST_PERMISSION_PROMPT_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "components/permissions/permission_prompt.h"
#include "content/public/browser/web_contents.h"

// PSST permission promt implementation
class PsstPermissionPromptImpl : public permissions::PermissionPrompt {
 public:
  PsstPermissionPromptImpl(const PsstPermissionPromptImpl&) = default;
  PsstPermissionPromptImpl& operator=(const PsstPermissionPromptImpl&) =
      default;
  PsstPermissionPromptImpl(content::WebContents* web_contents,
                           Delegate* delegate);
  ~PsstPermissionPromptImpl() override;

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
  void ShowCustomDialog();

  raw_ptr<content::WebContents> web_contents_;
  raw_ptr<Delegate> delegate_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_PSST_PERMISSION_PROMPT_IMPL_H_
