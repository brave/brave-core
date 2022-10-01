// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_google_sign_in_permission_context.h"

#include <utility>

#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request_id.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy.mojom.h"

namespace permissions {

BraveGoogleSignInPermissionContext::BraveGoogleSignInPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                            ContentSettingsType::BRAVE_GOOGLE_SIGN_IN,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

BraveGoogleSignInPermissionContext::~BraveGoogleSignInPermissionContext() =
    default;

bool BraveGoogleSignInPermissionContext::IsRestrictedToSecureOrigins() const {
  return false;
}

void BraveGoogleSignInPermissionContext::UpdateTabContext(
    const PermissionRequestID& id,
    const GURL& requesting_frame,
    bool allowed) {
  auto* content_settings =
      content_settings::PageSpecificContentSettings::GetForFrame(
          id.render_process_id(), id.render_frame_id());
  if (!content_settings) {
    return;
  }

  if (allowed) {
    content_settings->OnContentAllowed(
        ContentSettingsType::BRAVE_GOOGLE_SIGN_IN);
  } else {
    content_settings->OnContentBlocked(
        ContentSettingsType::BRAVE_GOOGLE_SIGN_IN);
  }
}
}  // namespace permissions
