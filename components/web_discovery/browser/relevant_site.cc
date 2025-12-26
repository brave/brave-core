// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_discovery/browser/relevant_site.h"

#include "base/containers/fixed_flat_map.h"

namespace web_discovery {

namespace {

constexpr std::string_view kGoogleImagesID = "search-goi";
constexpr std::string_view kGoogleVideosID = "search-gov";
constexpr std::string_view kGoogleID = "search-go";
constexpr std::string_view kYahooID = "search-ya";
constexpr std::string_view kBingImagesID = "search-bii";
constexpr std::string_view kBingID = "search-bi";
constexpr std::string_view kAmazonSearchID = "search-am";
constexpr std::string_view kAmazonProductID = "amp";
constexpr std::string_view kDuckDuckGoID = "search-dd";
constexpr std::string_view kLinkedInID = "li";

constexpr auto kIDToSiteMap =
    base::MakeFixedFlatMap<std::string_view, RelevantSite>({
        {kGoogleImagesID, RelevantSite::kGoogleImages},
        {kGoogleVideosID, RelevantSite::kGoogleVideos},
        {kGoogleID, RelevantSite::kGoogle},
        {kYahooID, RelevantSite::kYahoo},
        {kBingImagesID, RelevantSite::kBingImages},
        {kBingID, RelevantSite::kBing},
        {kAmazonSearchID, RelevantSite::kAmazonSearch},
        {kAmazonProductID, RelevantSite::kAmazonProduct},
        {kDuckDuckGoID, RelevantSite::kDuckDuckGo},
        {kLinkedInID, RelevantSite::kLinkedIn},
    });

constexpr auto kSiteToIDMap =
    base::MakeFixedFlatMap<RelevantSite, std::string_view>({
        {RelevantSite::kGoogleImages, kGoogleImagesID},
        {RelevantSite::kGoogleVideos, kGoogleVideosID},
        {RelevantSite::kGoogle, kGoogleID},
        {RelevantSite::kYahoo, kYahooID},
        {RelevantSite::kBingImages, kBingImagesID},
        {RelevantSite::kBing, kBingID},
        {RelevantSite::kAmazonSearch, kAmazonSearchID},
        {RelevantSite::kAmazonProduct, kAmazonProductID},
        {RelevantSite::kDuckDuckGo, kDuckDuckGoID},
        {RelevantSite::kLinkedIn, kLinkedInID},
    });

}  // namespace

std::optional<RelevantSite> RelevantSiteFromID(std::string_view text_id) {
  auto it = kIDToSiteMap.find(text_id);
  if (it != kIDToSiteMap.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<std::string_view> RelevantSiteToID(RelevantSite site) {
  auto it = kSiteToIDMap.find(site);
  if (it != kSiteToIDMap.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool IsRelevantSiteSearchEngine(RelevantSite site) {
  switch (site) {
    case RelevantSite::kGoogleImages:
    case RelevantSite::kGoogleVideos:
    case RelevantSite::kGoogle:
    case RelevantSite::kYahoo:
    case RelevantSite::kBingImages:
    case RelevantSite::kBing:
    case RelevantSite::kAmazonSearch:
    case RelevantSite::kDuckDuckGo:
      return true;
    case RelevantSite::kAmazonProduct:
    case RelevantSite::kLinkedIn:
      return false;
  }
}

}  // namespace web_discovery
