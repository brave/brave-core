// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_AI_CHAT_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_AI_CHAT_PERMISSION_CONTEXT_H_

#include "components/permissions/permission_context_base.h"
#include "content/public/browser/browser_context.h"

namespace permissions {

class BraveAIChatPermissionContext : public PermissionContextBase {
 public:
  explicit BraveAIChatPermissionContext(
      content::BrowserContext* browser_context);
  ~BraveAIChatPermissionContext() override;

  BraveAIChatPermissionContext(const BraveAIChatPermissionContext&) = delete;
  BraveAIChatPermissionContext& operator=(const BraveAIChatPermissionContext&) =
      delete;

  // PermissionContextBase:
  void RequestPermission(PermissionRequestData request_data,
                         BrowserPermissionCallback callback) override;

 private:
  bool IsRestrictedToSecureOrigins() const override;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_AI_CHAT_PERMISSION_CONTEXT_H_
