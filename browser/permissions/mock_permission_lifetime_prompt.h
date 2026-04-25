/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_MOCK_PERMISSION_LIFETIME_PROMPT_H_
#define BRAVE_BROWSER_PERMISSIONS_MOCK_PERMISSION_LIFETIME_PROMPT_H_

#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "components/permissions/permission_prompt.h"

namespace permissions {
class MockPermissionLifetimePromptFactory;

class MockPermissionLifetimePrompt : public PermissionPrompt {
 public:
  MockPermissionLifetimePrompt(MockPermissionLifetimePromptFactory* factory,
                               Delegate* delegate);
  ~MockPermissionLifetimePrompt() override;

  // PermissionPrompt:
  bool UpdateAnchor() override;
  TabSwitchingBehavior GetTabSwitchingBehavior() override;
  PermissionPromptDisposition GetPromptDisposition() const override;
  bool IsAskPrompt() const override;
  std::optional<gfx::Rect> GetViewBoundsInScreen() const override;
  bool ShouldFinalizeRequestAfterDecided() const override;
  std::vector<permissions::ElementAnchoredBubbleVariant> GetPromptVariants()
      const override;
  std::optional<feature_params::PermissionElementPromptPosition>
  GetPromptPosition() const override;

  Delegate* delegate() { return delegate_; }
  void ResetFactory();

 private:
  raw_ptr<MockPermissionLifetimePromptFactory> factory_ = nullptr;
  raw_ptr<Delegate> delegate_ = nullptr;
};

}  // namespace permissions

#endif  // BRAVE_BROWSER_PERMISSIONS_MOCK_PERMISSION_LIFETIME_PROMPT_H_
