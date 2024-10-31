// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_open_ai_chat_permission_context.h"

#include <utility>

#include "brave/brave_domains/service_domains.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request_data.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom-shared.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace permissions {

BraveOpenAIChatPermissionContext::BraveOpenAIChatPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                            ContentSettingsType::BRAVE_OPEN_AI_CHAT,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

BraveOpenAIChatPermissionContext::~BraveOpenAIChatPermissionContext() = default;

ContentSetting BraveOpenAIChatPermissionContext::GetPermissionStatusInternal(
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  // Check if origin is https://search.brave.com.
  if (!requesting_origin.SchemeIs(url::kHttpsScheme) ||
      requesting_origin.host_piece() !=
          brave_domains::GetServicesDomain("search")) {
    return ContentSetting::CONTENT_SETTING_BLOCK;
  }

  return PermissionContextBase::GetPermissionStatusInternal(
      render_frame_host, requesting_origin, embedding_origin);
}

bool BraveOpenAIChatPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}

}  // namespace permissions
