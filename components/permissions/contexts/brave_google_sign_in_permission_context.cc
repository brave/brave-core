// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_google_sign_in_permission_context.h"

#include "components/content_settings/core/common/content_settings_types.h"

namespace permissions {

BraveGoogleSignInPermissionContext::BraveGoogleSignInPermissionContext(
    content::BrowserContext* browser_context)
    : ContentSettingPermissionContextBase(
          browser_context,
          ContentSettingsType::BRAVE_GOOGLE_SIGN_IN,
          network::mojom::PermissionsPolicyFeature::kNotFound) {}

BraveGoogleSignInPermissionContext::~BraveGoogleSignInPermissionContext() =
    default;

bool BraveGoogleSignInPermissionContext::IsRestrictedToSecureOrigins() const {
  return false;
}
}  // namespace permissions
