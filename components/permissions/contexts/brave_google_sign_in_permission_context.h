// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_GOOGLE_SIGN_IN_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_GOOGLE_SIGN_IN_PERMISSION_CONTEXT_H_

#include "components/permissions/permission_context_base.h"
#include "content/public/browser/browser_context.h"

namespace permissions {

class BraveGoogleSignInPermissionContext : public PermissionContextBase {
 public:
  // using PermissionContextBase::RequestPermission;
  explicit BraveGoogleSignInPermissionContext(
      content::BrowserContext* browser_context);
  ~BraveGoogleSignInPermissionContext() override;

  BraveGoogleSignInPermissionContext(
      const BraveGoogleSignInPermissionContext&) = delete;
  BraveGoogleSignInPermissionContext& operator=(
      const BraveGoogleSignInPermissionContext&) = delete;

 private:
  bool IsRestrictedToSecureOrigins() const override;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_GOOGLE_SIGN_IN_PERMISSION_CONTEXT_H_
