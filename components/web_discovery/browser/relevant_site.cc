// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_discovery/browser/relevant_site.h"

namespace web_discovery {

std::optional<RelevantSite> RelevantSiteFromID(std::string_view text_id) {
  if (text_id == "search-goi") {
    return RelevantSite::kGoogleImages;
  } else if (text_id == "search-gov") {
    return RelevantSite::kGoogleVideos;
  } else if (text_id == "search-go") {
    return RelevantSite::kGoogle;
  } else if (text_id == "search-ya") {
    return RelevantSite::kYahoo;
  } else if (text_id == "search-bii") {
    return RelevantSite::kBingImages;
  } else if (text_id == "search-bi") {
    return RelevantSite::kBing;
  } else if (text_id == "search-am") {
    return RelevantSite::kAmazonSearch;
  } else if (text_id == "amp") {
    return RelevantSite::kAmazonProduct;
  } else if (text_id == "search-dd") {
    return RelevantSite::kDuckDuckGo;
  } else if (text_id == "li") {
    return RelevantSite::kLinkedIn;
  }
  return std::nullopt;
}

}  // namespace web_discovery
