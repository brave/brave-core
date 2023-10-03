/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_util.h"

#include <vector>

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_constants.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

absl::optional<SearchEngineInfo> FindSearchEngine(const GURL& url) {
  if (!url.is_valid()) {
    return absl::nullopt;
  }

  const GURL url_with_empty_path = url.GetWithEmptyPath();
  const std::vector<SearchEngineInfo>& search_engines = GetSearchEngines();
  for (const auto& search_engine : search_engines) {
    if (RE2::FullMatch(url_with_empty_path.spec(), search_engine.url_pattern) ||
        RE2::FullMatch(url.spec(), search_engine.url_pattern)) {
      return search_engine;
    }
  }

  return absl::nullopt;
}

}  // namespace

bool IsSearchEngine(const GURL& url) {
  return !!FindSearchEngine(url);
}

}  // namespace brave_ads
