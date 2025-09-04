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
  // In development builds, allow from any origin
  if (IS_DEVELOPMENT_BUILD) {
    return true;
  }
  
  // In production, only allow from search.brave.com
  return origin.host() == "search.brave.com" && origin.scheme() == "https";
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
  // Check if origin is allowed for puppeteer mode
  if (IsOriginAllowedForPuppeteerMode(url::Origin::Create(requesting_origin))) {
    // In development builds, auto-grant permission
    if (IS_DEVELOPMENT_BUILD) {
      return CONTENT_SETTING_ALLOW;
    }
    // In production, let the user decide (will show permission prompt)
    return CONTENT_SETTING_ASK;
  }
  return CONTENT_SETTING_BLOCK;
}
