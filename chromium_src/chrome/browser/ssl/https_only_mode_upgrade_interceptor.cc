/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_only_mode_upgrade_interceptor.h"

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"

class GURL;

namespace content {
class BrowserContext;
}  // namespace content

namespace {

bool ShouldUpgradeToHttps(content::BrowserContext* context, const GURL& url) {
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(context);
  return brave_shields::ShouldUpgradeToHttps(map, url);
}

}  // namespace

namespace net {
namespace {

bool IsOnion(const GURL& url) {
  return IsSubdomainOf(url.host(), "onion");
}

bool IsLocalhostOrOnion(const GURL& url) {
  return IsLocalhost(url) || IsOnion(url);
}

}  // namespace
}  // namespace net

#define IsLocalhost(URL) IsLocalhostOrOnion(URL)
#define GetBoolean(PREF_NAME) \
  GetBooleanOr(               \
      PREF_NAME,              \
      ShouldUpgradeToHttps(browser_context, tentative_resource_request.url))

#include "src/chrome/browser/ssl/https_only_mode_upgrade_interceptor.cc"

#undef IsLocalHost
#undef GetBoolean
