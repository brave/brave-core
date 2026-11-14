/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
namespace containers {
// Implemented in brave/browser/containers/containers_navigator_utils.cc.
std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToInherit(const NavigateParams& params);
}  // namespace containers
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace sidebar {

// kNewSplitView pairs the new tab with the active one, and the sidebar web
// panel's tab must never join a split. TabStripModel refuses such a split
// anyway, but this path dereferences AddToNewSplit()'s returned id for
// window.open() split requests, so it has to stop before calling it.
bool IsWebPanelTab(tabs::TabInterface* tab);

}  // namespace sidebar

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
      params->disposition == WindowOpenDisposition::kNewPopup) {
    params->disposition = WindowOpenDisposition::kNewForegroundTab;
  }
}

void UpdateParams(NavigateParams* params) {
  UpdateBraveScheme(params);
  MaybeOverridePopupDisposition(params);
}

std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToInherit(const NavigateParams& params) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (auto config = containers::GetStoragePartitionConfigToInherit(params)) {
    return config;
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return std::nullopt;
}

}  // namespace

#include <chrome/browser/ui/navigator/browser_navigator.cc>
