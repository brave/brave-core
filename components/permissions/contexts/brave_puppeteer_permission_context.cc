// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_puppeteer_permission_context.h"

#include "base/logging.h"
#include "brave/components/permissions/puppeteer_features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
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
    content::BrowserContext* browser_context, const url::Origin& origin) {
  LOG(INFO) << "[PUPPETEER_DEBUG] IsOriginAllowedForPuppeteerMode called for: " << origin;

  // Step 1: Check if puppeteer permission feature is enabled
  if (!permissions::features::IsBravePuppeteerPermissionEnabled()) {
    LOG(INFO) << "[PUPPETEER_DEBUG] Feature disabled, returning false";
    return false;
  }

  LOG(INFO) << "[PUPPETEER_DEBUG] Feature enabled, checking allowlist";

  // Step 2: Check if origin is in allowlist
  bool is_allowlisted = false;

  // In development builds, allow all hosts and localhost
  if (IS_DEVELOPMENT_BUILD) {
    is_allowlisted = true;
  }
  // Allow all brave:// scheme origins
  else if (origin.scheme() == "brave") {
    is_allowlisted = true;
  }
  // Allow search.brave.com if HTTPS
  else if (origin.host() == "search.brave.com" && origin.scheme() == "https") {
    is_allowlisted = true;
  }

  if (!is_allowlisted) {
    LOG(INFO) << "[PUPPETEER_DEBUG] Origin not in allowlist, returning false";
    return false;
  }

  LOG(INFO) << "[PUPPETEER_DEBUG] Origin in allowlist, checking permission setting";

  // Step 3: Check user permission setting
  if (!browser_context) {
    return false;
  }

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(browser_context);
  if (!settings_map) {
    return false;
  }

  GURL origin_url = origin.GetURL();
  ContentSetting permission_setting = settings_map->GetContentSetting(
      origin_url, origin_url, ContentSettingsType::BRAVE_PUPPETEER);

  // Allow if permission is explicitly granted or set to default (ASK)
  // In production, we want explicit user consent, but in development builds
  // we can be more permissive
  LOG(INFO) << "[PUPPETEER_DEBUG] Permission setting: " << permission_setting
            << ", is_development: " << IS_DEVELOPMENT_BUILD;

  bool result;
  if (IS_DEVELOPMENT_BUILD) {
    result = permission_setting != CONTENT_SETTING_BLOCK;
  } else {
    result = permission_setting == CONTENT_SETTING_ALLOW;
  }

  LOG(INFO) << "[PUPPETEER_DEBUG] Final result: " << (result ? "ALLOWED" : "DENIED");
  return result;
}

// Global function for chromium_src forward declaration
// This follows the same pattern as brave_policy components
namespace brave_puppeteer {

bool IsOriginAllowedForPuppeteerMode(content::BrowserContext* browser_context, const url::Origin& origin) {
  return BravePuppeteerPermissionContext::IsOriginAllowedForPuppeteerMode(browser_context, origin);
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
  if (!IsOriginAllowedForPuppeteerMode(browser_context(), url::Origin::Create(requesting_origin))) {
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
