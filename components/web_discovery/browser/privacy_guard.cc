/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/privacy_guard.h"

#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/web_discovery/browser/hash_detection.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "brave/components/web_discovery/browser/util.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace web_discovery {

namespace {

constexpr size_t kMaxSearchEngineRefLength = 8;
constexpr size_t kMaxQueryLength = 50;
constexpr size_t kMaxQueryNumberLength = 7;
constexpr size_t kMaxQuerySplitLength = 7;
constexpr size_t kMaxQueryWordLength = 20;
constexpr size_t kHashCheckMinimumLength = 13;
constexpr double kHashCheckThresholdMultiplier = 1.5;

constexpr size_t kMaxQueryStringLength = 30;
constexpr size_t kMaxQueryStringParts = 4;
constexpr size_t kMaxQueryStringOrPathNumberLength = 12;
constexpr size_t kMaxPathPartLength = 18;
constexpr size_t kMinPathPartHashCheckLength = 13;
constexpr size_t kMinSegmentHashCheckLength = 16;

constexpr size_t kMaxDotSplitDomainSize = 6;
constexpr size_t kMaxHyphenSplitDomainSize = 4;
constexpr size_t kMaxDomainNumberLength = 5;

constexpr char kDefaultSearchPrefix[] = "search?q=";

constexpr char kOnionSiteSuffix[] = ".onion";
constexpr char kLocalDomainSuffix[] = ".local";
constexpr char kLocalhost[] = "localhost";

constexpr char kGoogleHostSubstring[] = "google";
constexpr char kGoogleURLQueryParam[] = "url";
constexpr char kMaskedURLSuffix[] = "/ (PROTECTED)";

bool ContainsForbiddenKeywords(const GURL& url) {
  auto path_and_query = base::StrCat({url.path(), "?", url.query()});
  if (RegexUtil::GetInstance()->CheckPathAndQueryStringKeywords(
          path_and_query)) {
    return true;
  }
  if (!url.ref().empty() &&
      RegexUtil::GetInstance()->CheckQueryStringOrRefKeywords(
          base::StrCat({"#", url.ref()}))) {
    return true;
  }
  if (!url.query().empty() &&
      RegexUtil::GetInstance()->CheckQueryStringOrRefKeywords(
          base::StrCat({"?", url.query()}))) {
    return true;
  }
  return false;
}

bool IsPrivateDomainLikely(std::string_view host) {
  auto dot_split =
      base::SplitString(host, ".", base::WhitespaceHandling::KEEP_WHITESPACE,
                        base::SPLIT_WANT_ALL);
  if (dot_split.size() > kMaxDotSplitDomainSize) {
    return true;
  }
  if (RegexUtil::GetInstance()->CheckForLongNumber(host,
                                                   kMaxDomainNumberLength)) {
    return true;
  }
  auto hyphen_split =
      base::SplitString(host, "-", base::WhitespaceHandling::KEEP_WHITESPACE,
                        base::SPLIT_WANT_ALL);
  if (hyphen_split.size() > kMaxHyphenSplitDomainSize) {
    return true;
  }
  return false;
}

}  // namespace

bool IsPrivateURLLikely(const GURL& url,
                        const PatternsURLDetails* matching_url_details) {
  if (!url.SchemeIs("https")) {
    VLOG(1) << "Ignoring URL due to non-HTTPS scheme";
    return true;
  }
  if (url.HostIsIPAddress()) {
    VLOG(1) << "Ignoring URL due to IP address host";
    return true;
  }
  if (url.has_username() || url.has_password()) {
    VLOG(1) << "Ignoring URL due to inclusion of credentials";
    return true;
  }
  if (url.has_port() && url.port() != "443") {
    VLOG(1) << "Ignoring URL due to non-standard port";
    return true;
  }
  if (matching_url_details && matching_url_details->is_search_engine) {
    if (url.has_ref() && url.ref().length() > kMaxSearchEngineRefLength) {
      VLOG(1) << "Ignoring search engine URL due to long ref";
      return true;
    }
  }
  std::string_view host_piece = url.host();
  if (host_piece.ends_with(kOnionSiteSuffix) ||
      host_piece.ends_with(kLocalDomainSuffix) || host_piece == kLocalhost) {
    VLOG(1) << "Ignoring URL due a local host or onion site";
    return true;
  }
  if (IsPrivateDomainLikely(url.host())) {
    VLOG(1) << "Ignoring URL due likely private domain";
    return true;
  }
  return false;
}

bool IsPrivateQueryLikely(const std::string& query) {
  if (query.length() > kMaxQueryLength) {
    VLOG(1) << "Ignoring query due to long length";
    return true;
  }
  auto split =
      base::SplitString(query, " ", base::WhitespaceHandling::KEEP_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);
  if (split.size() > kMaxQuerySplitLength) {
    VLOG(1) << "Ignoring query due to long split length";
    return true;
  }
  if (RegexUtil::GetInstance()->CheckForLongNumber(query,
                                                   kMaxQueryNumberLength)) {
    VLOG(1) << "Ignoring query due to long number";
    return true;
  }
  if (RegexUtil::GetInstance()->CheckForEmail(query)) {
    VLOG(1) << "Ignoring query due to inclusion of email";
    return true;
  }
  if (RegexUtil::GetInstance()->CheckQueryHTTPCredentials(query)) {
    VLOG(1) << "Ignoring query due to potential inclusion of HTTP credentials";
    return true;
  }
  for (const auto& word : split) {
    if (word.length() > kMaxQueryWordLength) {
      VLOG(1) << "Ignoring query due to long word";
      return true;
    }
  }
  if (query.length() >= kHashCheckMinimumLength) {
    if (IsHashLikely(query, kHashCheckThresholdMultiplier)) {
      VLOG(1) << "Ignoring query due to potential inclusion of hash";
      return true;
    }
  }
  return false;
}

GURL GeneratePrivateSearchURL(const GURL& original_url,
                              const std::string& query,
                              const PatternsURLDetails& matching_url_details) {
  url::RawCanonOutputT<char> query_encoded;
  url::EncodeURIComponent(query, &query_encoded);
  std::string query_encoded_str(query_encoded.view());
  base::ReplaceSubstringsAfterOffset(&query_encoded_str, 0, "%20", "+");

  return GURL(
      base::StrCat({original_url.scheme(), url::kStandardSchemeSeparator,
                    original_url.host(), "/",
                    matching_url_details.search_template_prefix.value_or(
                        kDefaultSearchPrefix),
                    query_encoded_str}));
}

bool ShouldMaskURL(const GURL& url) {
  if (RegexUtil::GetInstance()->CheckForEmail(url.spec())) {
    return true;
  }
  if (!url.query().empty()) {
    if (url.query().size() > kMaxQueryStringLength) {
      return true;
    }
    auto query_parts = base::SplitStringPiece(
        url.query(), "&;", base::WhitespaceHandling::KEEP_WHITESPACE,
        base::SplitResult::SPLIT_WANT_ALL);
    if (query_parts.size() > kMaxQueryStringParts) {
      return true;
    }
    if (RegexUtil::GetInstance()->CheckForLongNumber(
            url.query(), kMaxQueryStringOrPathNumberLength)) {
      return true;
    }
  }
  if (!url.path().empty()) {
    if (RegexUtil::GetInstance()->CheckForLongNumber(
            url.path(), kMaxQueryStringOrPathNumberLength)) {
      return true;
    }
  }
  auto path_parts = base::SplitStringPiece(
      url.path(), "/._ -:+;", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
  for (const auto& path_part : path_parts) {
    if (path_part.length() > kMaxPathPartLength) {
      return true;
    }
    if (path_part.length() >= kMinPathPartHashCheckLength &&
        IsHashLikely(path_part)) {
      return true;
    }
  }
  auto path_segments = base::SplitStringPiece(
      url.path(), "/", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
  for (const auto& path_segment : path_segments) {
    std::string alphanumeric_path_segment = std::string(path_segment);
    TransformToAlphanumeric(alphanumeric_path_segment);
    if (alphanumeric_path_segment.length() >= kMinSegmentHashCheckLength &&
        IsHashLikely(alphanumeric_path_segment)) {
      return true;
    }
  }
  return ContainsForbiddenKeywords(url);
}

std::optional<std::string> MaskURL(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS() || !url.is_valid()) {
    return std::nullopt;
  }

  if (!ShouldMaskURL(url)) {
    return url.spec();
  }

  if (url.host().find(kGoogleHostSubstring) != std::string::npos &&
      url.has_query()) {
    auto google_url_param =
        ExtractValueFromQueryString(url.query(), kGoogleURLQueryParam);
    if (google_url_param) {
      GURL decoded_embedded_url(*google_url_param);
      if (!decoded_embedded_url.is_valid()) {
        return std::nullopt;
      }
      return MaskURL(decoded_embedded_url);
    }
  }

  return base::StrCat({url.scheme(), url::kStandardSchemeSeparator, url.host(),
                       kMaskedURLSuffix});
}

}  // namespace web_discovery
