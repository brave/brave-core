/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "content/public/common/url_constants.h"
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

void MaybeOverridePopupDisposition(NavigateParams* params) {
  if (base::FeatureList::IsEnabled(features::kForcePopupToBeOpenedAsTab) &&
      params->disposition == WindowOpenDisposition::NEW_POPUP) {
    params->disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  }
}

void UpdateParams(NavigateParams* params) {
  UpdateBraveScheme(params);
  MaybeOverridePopupDisposition(params);
}

}  // namespace

#define BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL UpdateParams(params);

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
