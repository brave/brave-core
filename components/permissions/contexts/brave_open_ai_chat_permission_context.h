// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_OPEN_AI_CHAT_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_OPEN_AI_CHAT_PERMISSION_CONTEXT_H_

#include "components/permissions/permission_context_base.h"
#include "content/public/browser/browser_context.h"

namespace permissions {

class BraveOpenAIChatPermissionContext : public PermissionContextBase {
 public:
  explicit BraveOpenAIChatPermissionContext(
      content::BrowserContext* browser_context);
  ~BraveOpenAIChatPermissionContext() override;

  BraveOpenAIChatPermissionContext(const BraveOpenAIChatPermissionContext&) =
      delete;
  BraveOpenAIChatPermissionContext& operator=(
      const BraveOpenAIChatPermissionContext&) = delete;

 private:
  // PermissionContextBase:
  ContentSetting GetPermissionStatusInternal(
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      const GURL& embedding_origin) const override;
  bool IsRestrictedToSecureOrigins() const override;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_OPEN_AI_CHAT_PERMISSION_CONTEXT_H_
