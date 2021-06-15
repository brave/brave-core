// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/brave_search_utils.h"

#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/strings/string_piece_forward.h"
#include "brave/components/brave_search/common/features.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

constexpr auto kVettedHosts = base::MakeFixedFlatSet<base::StringPiece>(
    {"search.brave.com", "search-dev.brave.com", "search-dev-local.brave.com",
     "search.brave.software", "search.bravesoftware.com"});

}  // namespace

namespace brave_search {

bool IsAllowedHost(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }
  std::string host = url.host();
  return base::Contains(kVettedHosts, host);
}

bool IsDefaultAPIEnabled() {
  return base::FeatureList::IsEnabled(
      brave_search::features::kBraveSearchDefaultAPIFeature);
}

}  // namespace brave_search
