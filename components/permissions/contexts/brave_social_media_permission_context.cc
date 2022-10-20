// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/permissions/contexts/brave_social_media_permission_context.h"
#include <utility>

#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request_id.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy.mojom.h"

namespace permissions {

BraveSocialMediaPermissionContext::BraveSocialMediaPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                            ContentSettingsType::BRAVE_GOOGLE_SIGN_IN,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

BraveSocialMediaPermissionContext::~BraveSocialMediaPermissionContext() =
    default;

bool BraveSocialMediaPermissionContext::IsRestrictedToSecureOrigins() const {
  return false;
}

void BraveSocialMediaPermissionContext::UpdateTabContext(
    const PermissionRequestID& id,
    const GURL& requesting_frame,
    bool allowed) {
  auto* content_settings =
      content_settings::PageSpecificContentSettings::GetForFrame(
          id.render_process_id(), id.render_frame_id());
  if (!content_settings)
    return;

  if (allowed)
    content_settings->OnContentAllowed(
        ContentSettingsType::BRAVE_GOOGLE_SIGN_IN);
  else
    content_settings->OnContentBlocked(
        ContentSettingsType::BRAVE_GOOGLE_SIGN_IN);
}
}  // namespace permissions
