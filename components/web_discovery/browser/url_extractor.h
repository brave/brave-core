/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_URL_EXTRACTOR_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_URL_EXTRACTOR_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/web_discovery/browser/relevant_site.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace web_discovery {

// Details about a relevant site pattern
struct RelevantSiteDetails {
  RelevantSiteDetails(RelevantSite site,
                      bool is_search_engine,
                      std::unique_ptr<re2::RE2>&& regex,
                      std::vector<std::string_view>&& query_params,
                      std::optional<std::string_view> prefix);
  ~RelevantSiteDetails();

  RelevantSiteDetails(const RelevantSiteDetails&) = delete;
  RelevantSiteDetails& operator=(const RelevantSiteDetails&) = delete;

  RelevantSiteDetails(RelevantSiteDetails&&);
  RelevantSiteDetails& operator=(RelevantSiteDetails&&);

  RelevantSite site;
  bool is_search_engine;
  std::unique_ptr<re2::RE2> regex;
  // List of query parameters to process in order to extract the query
  std::vector<std::string_view> query_params;
  // Used for generating a private search URL
  std::optional<std::string_view> private_query_prefix;
};

// Result of URL identification
struct URLExtractResult {
  URLExtractResult(const RelevantSiteDetails* details,
                   std::optional<std::string>&& query);
  ~URLExtractResult();

  URLExtractResult(const URLExtractResult&) = delete;
  URLExtractResult& operator=(const URLExtractResult&) = delete;

  URLExtractResult(URLExtractResult&&);
  URLExtractResult& operator=(URLExtractResult&&);

  raw_ptr<const RelevantSiteDetails> details;
  std::optional<std::string> query;
};

// URLExtractor provides functionality for identifying URLs and extracting
// search queries from them. This is only used for v2 patterns.
class URLExtractor {
 public:
  URLExtractor();
  ~URLExtractor();

  URLExtractor(const URLExtractor&) = delete;
  URLExtractor& operator=(const URLExtractor&) = delete;

  URLExtractor(URLExtractor&&);
  URLExtractor& operator=(URLExtractor&&);

  // Identifies a URL and extracts query if present
  std::optional<URLExtractResult> IdentifyURL(const GURL& url) const;

 private:
  // Initializes the URL patterns
  void InitializePatterns();

  // Extracts query parameter from URL using the site details
  std::optional<std::string> ExtractQuery(
      const GURL& url,
      const RelevantSiteDetails& details) const;

  std::vector<RelevantSiteDetails> site_details_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_URL_EXTRACTOR_H_
