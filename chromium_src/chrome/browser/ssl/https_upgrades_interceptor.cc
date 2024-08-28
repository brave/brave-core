/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_upgrades_interceptor.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

// Prevent double-defining macro
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"

#define MaybeCreateLoader(...)                                                \
  MaybeCreateLoader(__VA_ARGS__) {                                            \
    if (brave_shields::IsHttpsByDefaultFeatureEnabled()) {                    \
      HostContentSettingsMap* map =                                           \
          HostContentSettingsMapFactory::GetForProfile(browser_context);      \
      if (!map ||                                                             \
          !brave_shields::ShouldUpgradeToHttps(                               \
              map, tentative_resource_request.url,                            \
              g_brave_browser_process->https_upgrade_exceptions_service())) { \
        std::move(callback).Run({});                                          \
        return;                                                               \
      }                                                                       \
      http_interstitial_enabled_by_pref_ = brave_shields::ShouldForceHttps(   \
          map, tentative_resource_request.url);                               \
    }                                                                         \
    MaybeCreateLoader_ChromiumImpl(tentative_resource_request,                \
                                   browser_context, std::move(callback));     \
  }                                                                           \
  void HttpsUpgradesInterceptor::MaybeCreateLoader_ChromiumImpl(__VA_ARGS__)

#define IsEnabled(FLAG)                                \
  IsEnabled(FLAG.name == features::kHttpsUpgrades.name \
                ? net::features::kBraveHttpsByDefault  \
                : FLAG)

#define IsLocalhost(URL) IsLocalhostOrOnion(URL)

#include "src/chrome/browser/ssl/https_upgrades_interceptor.cc"

#undef MaybeCreateLoader
#undef IsEnabled
#undef IsLocalhost
