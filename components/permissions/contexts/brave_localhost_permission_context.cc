// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_localhost_permission_context.h"

#include <utility>

#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request_id.h"

namespace permissions {

BraveLocalhostPermissionContext::BraveLocalhostPermissionContext(
    content::BrowserContext* browser_context)
    : ContentSettingPermissionContextBase(
          browser_context,
          ContentSettingsType::BRAVE_LOCALHOST_ACCESS,
          network::mojom::PermissionsPolicyFeature::kNotFound) {}

BraveLocalhostPermissionContext::~BraveLocalhostPermissionContext() = default;

bool BraveLocalhostPermissionContext::IsRestrictedToSecureOrigins() const {
  return false;
}
}  // namespace permissions
