// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_FEATURES_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_FEATURES_H_

#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.h"

class Profile;

namespace brave_welcome_page {

// Returns the features that can be offered to the user in this profile.
// Features that are unavailable in this build, unsupported for this profile or
// region, or disabled by policy are not returned.
base::flat_set<mojom::Feature> GetAvailableFeatures(Profile* profile);

// Returns the prefs that control the visibility of the feature's entry points.
std::vector<std::string_view> GetFeatureVisibilityPrefs(mojom::Feature feature);

}  // namespace brave_welcome_page

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_FEATURES_H_
