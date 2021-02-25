/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_MOCK_PERMISSION_LIFETIME_PROMPT_H_
#define BRAVE_BROWSER_PERMISSIONS_MOCK_PERMISSION_LIFETIME_PROMPT_H_

#include "components/permissions/permission_prompt.h"

namespace permissions {
class MockPermissionLifetimePromptFactory;

class MockPermissionLifetimePrompt : public PermissionPrompt {
 public:
  MockPermissionLifetimePrompt(MockPermissionLifetimePromptFactory* factory,
                               Delegate* delegate);
  ~MockPermissionLifetimePrompt() override;

  // PermissionPrompt:
  void UpdateAnchorPosition() override;
  TabSwitchingBehavior GetTabSwitchingBehavior() override;
  PermissionPromptDisposition GetPromptDisposition() const override;

  Delegate* delegate() { return delegate_; }
  void ResetFactory();

 private:
  MockPermissionLifetimePromptFactory* factory_;
  Delegate* delegate_;
};

}  // namespace permissions

#endif  // BRAVE_BROWSER_PERMISSIONS_MOCK_PERMISSION_LIFETIME_PROMPT_H_
