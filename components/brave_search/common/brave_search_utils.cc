// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/brave_search_utils.h"

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "brave/components/brave_search/common/features.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_search {

bool IsAllowedHost(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }
  return kVettedHosts.contains(url.host_piece());
}

bool IsDefaultAPIEnabled() {
  return base::FeatureList::IsEnabled(
      brave_search::features::kBraveSearchDefaultAPIFeature);
}

}  // namespace brave_search
