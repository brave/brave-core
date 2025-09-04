// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_puppeteer_permission_context.h"

#include "base/logging.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "url/gurl.h"

#if DCHECK_IS_ON()
#define IS_DEVELOPMENT_BUILD true
#else
#define IS_DEVELOPMENT_BUILD false
#endif

BravePuppeteerPermissionContext::BravePuppeteerPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                           ContentSettingsType::BRAVE_PUPPETEER,
                           network::mojom::PermissionsPolicyFeature::kNotFound) {}

BravePuppeteerPermissionContext::~BravePuppeteerPermissionContext() = default;

bool BravePuppeteerPermissionContext::IsOriginAllowedForPuppeteerMode(
    const url::Origin& origin) {
  // In development builds, allow all hosts and localhost
  if (IS_DEVELOPMENT_BUILD) {
    return true;
  }
  
  // Allow all brave:// scheme origins
  if (origin.scheme() == "brave") {
    return true;
  }
  
  // Allow search.brave.com if HTTPS
  if (origin.host() == "search.brave.com" && origin.scheme() == "https") {
    return true;
  }
  
  return false;
}

// Global function for chromium_src forward declaration
// This follows the same pattern as brave_policy components
namespace brave_puppeteer {

bool IsOriginAllowedForPuppeteerMode(const url::Origin& origin) {
  return BravePuppeteerPermissionContext::IsOriginAllowedForPuppeteerMode(origin);
}

} // namespace brave_puppeteer

bool BravePuppeteerPermissionContext::IsRestrictedToSecureOrigins() const {
  // In development builds, allow insecure origins (localhost, etc.)
  if (IS_DEVELOPMENT_BUILD) {
    return false;
  }
  
  // In production, require secure origins
  return true;
}

PermissionSetting BravePuppeteerPermissionContext::GetPermissionStatusInternal(
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  LOG(INFO) << "[PUPPETEER_DEBUG] GetPermissionStatusInternal called for origin: "
            << requesting_origin << ", embedding: " << embedding_origin;

  // Check if origin is allowed for puppeteer mode
  if (!IsOriginAllowedForPuppeteerMode(url::Origin::Create(requesting_origin))) {
    LOG(INFO) << "[PUPPETEER_DEBUG] Origin not allowed, returning BLOCK";
    return CONTENT_SETTING_BLOCK;
  }

  // Origin is allowed, get the actual stored permission setting
  LOG(INFO) << "[PUPPETEER_DEBUG] Origin allowed, getting stored setting";
  PermissionSetting stored_setting = PermissionContextBase::GetPermissionStatusInternal(
      render_frame_host, requesting_origin, embedding_origin);

  LOG(INFO) << "[PUPPETEER_DEBUG] Stored setting: " << stored_setting;
  return stored_setting;
}
