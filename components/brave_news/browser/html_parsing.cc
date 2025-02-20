// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/html_parsing.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/i18n/icu_string_conversions.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace brave_news {

namespace {

constexpr auto kSupportedFeedTypes =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "application/atom+xml",
                                                 "application/json",
                                                 "application/rss+atom",
                                                 "application/rss+xml",
                                                 "application/xml",
                                             });

constexpr auto kSupportedRels =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "alternate",
                                                 "service.feed",
                                             });

}  // namespace

std::vector<GURL> GetFeedURLsFromHTMLDocument(const std::string& charset,
                                              const std::string& raw_body,
                                              const GURL& html_url) {
  std::string html_body;
  if (!base::ConvertToUtf8AndNormalize(raw_body, charset, &html_body)) {
    return std::vector<GURL>();
  }

  VLOG(1) << "GetFeedURLsFromHTMLDocument";
  std::vector<GURL> results;
  // Find most `<link` elements from most types of html documents
  static const re2::RE2 link_pattern("(?i)(<\\s*link [^>]+>)");
  std::string link_text;
  std::string_view input(html_body);
  while (re2::RE2::FindAndConsume(&input, link_pattern, &link_text)) {
    VLOG(1) << "Found link: " << link_text;
    // Extract rel
    static const re2::RE2 rel_pattern("(?i)rel=\"([^\"]*)\"");
    std::string rel;
    if (!rel_pattern.PartialMatch(link_text, rel_pattern, &rel)) {
      VLOG(1) << "no valid matching rel: " << link_text;
      continue;
    }
    // Validate rel
    if (!base::IsStringASCII(rel) || !base::Contains(kSupportedRels, rel)) {
      VLOG(1) << "not valid rel: " << rel;
      continue;
    }
    // Extract type attribute
    static const re2::RE2 type_pattern("(?i)type=\"([^\"]+)\"");
    std::string content_type;
    if (!type_pattern.PartialMatch(link_text, type_pattern, &content_type)) {
      VLOG(1) << "no valid matching type: " << link_text;
      continue;
    }
    // Validate type
    if (!base::IsStringASCII(content_type) ||
        !base::Contains(kSupportedFeedTypes, content_type)) {
      VLOG(1) << "not valid type: " << content_type;
      continue;
    }
    // Extract href attribute
    static const re2::RE2 href_pattern("(?i)href=\"([^\"]+)\"");
    std::string href;
    if (!href_pattern.PartialMatch(link_text, href_pattern, &href)) {
      VLOG(1) << "no valid href: " << link_text;
      continue;
    }
    // Validate href
    if (href.empty() || !base::IsStringASCII(href)) {
      VLOG(1) << "not valid href: " << href;
      continue;
    }
    // Validate url
    auto feed_url = html_url.Resolve(href);
    if (!feed_url.is_valid()) {
      VLOG(1) << "feed url not valid: " << href;
      continue;
    }
    results.emplace_back(feed_url);
  }
  return results;
}

}  // namespace brave_news
