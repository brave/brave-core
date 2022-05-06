/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_INFO_H_

#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"

namespace content {
class RenderFrameHost;
}

namespace brave_ads {

enum class SearchResultAdState {
  kNotReady,
  kReadyForView,
  kNotCountView,
  kReadyForClick
};

struct SearchResultAdInfo {
  SearchResultAdInfo();
  SearchResultAdInfo(SearchResultAdInfo&& info);
  SearchResultAdInfo& operator=(SearchResultAdInfo&& info);
  ~SearchResultAdInfo();

  ads::mojom::SearchResultAdPtr ad;
  SearchResultAdState state = SearchResultAdState::kNotReady;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_INFO_H_
