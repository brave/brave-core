/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/browser_navigator_params.h"
// Needed to prevent overriding url_typed_with_http_scheme
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/contained_tab_handler_registry.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace {

void UpdateBraveScheme(NavigateParams* params) {
  if (params->url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    params->url = params->url.ReplaceComponents(replacements);
  }
}

bool IsURLAllowedInIncognitoBraveImpl(const GURL& url) {
  std::string scheme = url.scheme();
  std::string_view host = url.host_piece();
  if (scheme != content::kChromeUIScheme) {
    return true;
  }

  if (host == kRewardsPageHost || host == chrome::kChromeUISyncInternalsHost ||
      host == chrome::kBraveUISyncHost || host == kAdblockHost ||
      host == kWelcomeHost || host == kBraveGettingStartedHost) {
    return false;
  }

  return true;
}

}  // namespace

// We want URLs that were manually typed with HTTP scheme to be HTTPS
// upgradable, but preserve the upstream's behavior in regards to captive
// portals (like hotel login pages which typically aren't cofnigured to work
// with HTTPS)
#define url_typed_with_http_scheme \
  url_typed_with_http_scheme;      \
  force_no_https_upgrade = false

#define BRAVE_IS_URL_ALLOWED_IN_INCOGNITO     \
  if (!IsURLAllowedInIncognitoBraveImpl(url)) \
    return false;

#define BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL UpdateBraveScheme(params);

#if BUILDFLAG(ENABLE_CONTAINERS)
#define GetSiteInstanceForNewTab(...)                                    \
  GetSiteInstanceForNewTab(                                              \
      __VA_ARGS__,                                                       \
      containers::ContainedTabHandlerRegistry::GetInstance()             \
          .MaybeInheritStoragePartition(params.storage_partition_config, \
                                        params.source_site_instance.get()))
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/ui/browser_navigator.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#undef BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL
#undef BRAVE_IS_URL_ALLOWED_IN_INCOGNITO
#undef url_typed_with_http_scheme
