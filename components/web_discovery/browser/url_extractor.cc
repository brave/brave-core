// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_discovery/browser/url_extractor.h"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/web_discovery/browser/util.h"

namespace {

struct PreRelevantSiteDetails {
  web_discovery::RelevantSite site;
  bool is_search_engine;
  std::string_view regex_pattern;
  std::array<std::optional<std::string_view>, 2> query_params;
  std::optional<std::string_view> prefix;
};

constexpr std::array<PreRelevantSiteDetails, 10> kPreRelevantSiteDetails = {{
    {web_discovery::RelevantSite::kGoogleImages,
     true,
     "^https://[^/]*[.]google[.].*?[#?&;]((q=[^&]+&([^&]+&)*tbm=isch)|"
     "(tbm=isch&([^&]+&)*q=[^&]+))",
     {"q"},
     "search?tbm=isch&gbv=1&q="},
    {web_discovery::RelevantSite::kGoogleVideos,
     true,
     "^https://[^/]*[.]google[.].*?[#?&;]((q=[^&]+&([^&]+&)*tbm=vid)|"
     "(tbm=vid&([^&]+&)*q=[^&]+))",
     {"q"},
     "search?tbm=vid&gbv=1&q="},
    {web_discovery::RelevantSite::kGoogle,
     true,
     "^https://[^/]*[.]google[.].*?[#?&;]",
     {"q"},
     "search?q="},
    {web_discovery::RelevantSite::kYahoo,
     true,
     "^https://[^/]*[.]search[.]yahoo[.].*?[#?&;][pq]=[^$&]+",
     {"q", "p"},
     "search?q="},
    {web_discovery::RelevantSite::kBingImages,
     true,
     "^https://[^/]*[.]bing[.][^/]+/images/search[?]q=[^$&]+",
     {"q"},
     "images/search?q="},
    {web_discovery::RelevantSite::kBing,
     true,
     "^https://[^/]*[.]bing[.].*?[#?&;]q=[^$&]+",
     {"q"},
     "search?q="},
    {web_discovery::RelevantSite::kAmazonSearch,
     false,
     "^https://[^/]*[.]amazon[.][^/]+/(s[?]k=[^$&]+|.*[?&]field-keywords="
     "[^$&]+)",
     {"field-keywords", "k"},
     "s/?field-keywords="},
    {web_discovery::RelevantSite::kAmazonProduct,
     false,
     "^https://[^/]*[.]amazon[.][^/]+/(/dp/|/gp/product/)",
     {"keywords"},
     std::nullopt},
    {web_discovery::RelevantSite::kDuckDuckGo,
     true,
     "^https://duckduckgo.com/(?:html$|.*[?&]q=[^&]+.*&ia=web|[?]q=[^&]+$)",
     {"q"},
     "?q="},
    {web_discovery::RelevantSite::kLinkedIn,
     false,
     "^https://[^/]*linkedin[.][^/]+/pub/dir+",
     {},
     std::nullopt},
}};

}  // namespace

namespace web_discovery {

URLExtractResult::URLExtractResult(const RelevantSiteDetails* details,
                                   std::optional<std::string>&& query)
    : details(details), query(std::move(query)) {}
URLExtractResult::~URLExtractResult() = default;

URLExtractResult::URLExtractResult(URLExtractResult&&) = default;
URLExtractResult& URLExtractResult::operator=(URLExtractResult&&) = default;

RelevantSiteDetails::RelevantSiteDetails(
    RelevantSite site,
    bool is_search_engine,
    std::unique_ptr<re2::RE2>&& regex,
    std::vector<std::string_view>&& query_params,
    std::optional<std::string_view> prefix)
    : site(site),
      is_search_engine(is_search_engine),
      regex(std::move(regex)),
      query_params(std::move(query_params)),
      private_query_prefix(prefix) {}
RelevantSiteDetails::~RelevantSiteDetails() = default;

RelevantSiteDetails::RelevantSiteDetails(RelevantSiteDetails&&) = default;
RelevantSiteDetails& RelevantSiteDetails::operator=(RelevantSiteDetails&&) =
    default;

URLExtractor::URLExtractor() {
  InitializePatterns();
}

URLExtractor::~URLExtractor() = default;

void URLExtractor::InitializePatterns() {
  site_details_.reserve(kPreRelevantSiteDetails.size());

  for (const auto& pattern_data : kPreRelevantSiteDetails) {
    std::vector<std::string_view> query_params;
    for (const auto& param : pattern_data.query_params) {
      if (param) {
        query_params.emplace_back(*param);
      }
    }

    auto regex = std::make_unique<re2::RE2>(pattern_data.regex_pattern);
    CHECK(regex->ok());

    site_details_.emplace_back(pattern_data.site, pattern_data.is_search_engine,
                               std::move(regex), std::move(query_params),
                               pattern_data.prefix);
  }
}

std::optional<URLExtractResult> URLExtractor::IdentifyURL(
    const GURL& url) const {
  if (!url.is_valid()) {
    return std::nullopt;
  }

  for (const auto& details : site_details_) {
    if (re2::RE2::PartialMatch(url.spec(), *details.regex)) {
      return std::make_optional<URLExtractResult>(&details,
                                                  ExtractQuery(url, details));
    }
  }

  return std::nullopt;
}

std::optional<std::string> URLExtractor::ExtractQuery(
    const GURL& url,
    const RelevantSiteDetails& details) const {
  if (!url.has_query() || details.query_params.empty()) {
    return std::nullopt;
  }

  std::string query_string = url.query();

  base::ReplaceSubstringsAfterOffset(&query_string, 0, "+", "%20");

  // Try each query parameter for this site
  for (const auto& param : details.query_params) {
    auto result = ExtractValueFromQueryString(query_string, param);
    if (result && !result->empty()) {
      return result;
    }
  }

  return std::nullopt;
}

}  // namespace web_discovery
