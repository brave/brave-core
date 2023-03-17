// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_LOCALHOST_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_LOCALHOST_PERMISSION_CONTEXT_H_

#include "components/permissions/permission_context_base.h"
#include "content/public/browser/browser_context.h"

namespace permissions {

class BraveLocalhostPermissionContext : public PermissionContextBase {
 public:
  // using PermissionContextBase::RequestPermission;
  explicit BraveLocalhostPermissionContext(
      content::BrowserContext* browser_context);
  ~BraveLocalhostPermissionContext() override;

  BraveLocalhostPermissionContext(const BraveLocalhostPermissionContext&) =
      delete;
  BraveLocalhostPermissionContext& operator=(
      const BraveLocalhostPermissionContext&) = delete;

 private:
  bool IsRestrictedToSecureOrigins() const override;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_LOCALHOST_PERMISSION_CONTEXT_H_
