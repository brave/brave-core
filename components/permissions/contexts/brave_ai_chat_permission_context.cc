// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_ai_chat_permission_context.h"

#include <utility>

#include "brave/brave_domains/service_domains.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request_data.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom-shared.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace permissions {

BraveAIChatPermissionContext::BraveAIChatPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                            ContentSettingsType::BRAVE_AI_CHAT,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

BraveAIChatPermissionContext::~BraveAIChatPermissionContext() = default;

void BraveAIChatPermissionContext::RequestPermission(
    PermissionRequestData request_data,
    BrowserPermissionCallback callback) {
  // Check if origin is https://search.brave.com.
  GURL& origin = request_data.requesting_origin;
  if (origin.SchemeIs(url::kHttpsScheme) &&
      origin.host_piece() == brave_domains::GetServicesDomain("search")) {
    PermissionContextBase::RequestPermission(std::move(request_data),
                                             std::move(callback));
    return;
  }

  // Deny permission for all other origins.
  std::move(callback).Run(ContentSetting::CONTENT_SETTING_BLOCK);
}

bool BraveAIChatPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}

}  // namespace permissions
