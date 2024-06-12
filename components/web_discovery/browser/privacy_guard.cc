/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/privacy_guard.h"

#include <array>

#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/web_discovery/browser/hash_detection.h"
#include "brave/components/web_discovery/browser/util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/third_party/mozilla/url_parse.h"
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

constexpr char kLongNumberRegexPrefix[] = "[0-9]{";
constexpr char kLongNumberRegexSuffix[] = ",}";
constexpr char kEmailRegex[] =
    "[a-z0-9\\-_@]+(@|%40|%(25)+40)[a-z0-9\\-_]+\\.[a-z0-9\\-_]";
constexpr char kHttpPasswordRegex[] = "[^:]+:[^@]+@";
constexpr char kDefaultSearchPrefix[] = "search?q=";

constexpr char kOnionSiteSuffix[] = ".onion";
constexpr char kLocalDomainSuffix[] = ".local";
constexpr char kLocalhost[] = "localhost";

constexpr char kGoogleHostSubstring[] = "google";
constexpr char kGoogleURLQueryParam[] = "url";
constexpr char kMaskedURLSuffix[] = "/ (PROTECTED)";

constexpr std::array<std::string_view, 10> kPathAndQueryStringCheckRegexes = {
    "(?i)\\/admin([\\/\\?#=]|$)",
    "(?i)\\/wp-admin([\\/\\?#=]|$)",
    "(?i)\\/edit([\\/\\?#=]|$)",
    "(?i)[&\\?#\\/]share([\\/\\?#=]|$)",
    "(?i)[&\\?#\\/;]sharing([\\/\\?#=]|$)",
    "(?i)[&\\?#\\/;]logout([\\/\\?#=]|$)",
    "(?i)WebLogic",
    "(?i)[&\\?#\\/;]token([\\/\\?#=_;]|$)",
    "(?i)[&\\?#\\/;]trk([\\/\\?#=_]|$)",
    "[&\\?#\\/=;](http|https)(:\\/|\\%3A\\%2F)"};

constexpr std::array<std::string_view, 20> kQueryStringAndRefCheckRegexes = {
    "(?i)[&\\?#_\\-;]user",     "(?i)[&\\?#_\\-;]token",
    "(?i)[&\\?#_\\-;]auth",     "(?i)[&\\?#_\\-;]uid",
    "(?i)[&\\?#_\\-;]email",    "(?i)[&\\?#_\\-;]usr",
    "(?i)[&\\?#_\\-;]pin",      "(?i)[&\\?#_\\-;]pwd",
    "(?i)[&\\?#_\\-;]password", "(?i)[&\\?#;]u[=#]",
    "(?i)[&\\?#;]url[=#]",      "(?i)[&\\?#_\\-;]http",
    "(?i)[&\\?#_\\-;]ref[=#]",  "(?i)[&\\?#_\\-;]red[=#]",
    "(?i)[&\\?#_\\-;]trk",      "(?i)[&\\?#_\\-;]track",
    "(?i)[&\\?#_\\-;]shar",     "(?i)[&\\?#_\\-;]login",
    "(?i)[&\\?#_\\-;]logout",   "(?i)[&\\?#_\\-;]session",
};

bool CheckForEmail(const std::string& str) {
  re2::RE2 email_regex(kEmailRegex);
  return re2::RE2::PartialMatch(str, email_regex);
}

bool CheckForLongNumber(const std::string_view str, size_t max_length) {
  auto regex_str = base::StrCat({kLongNumberRegexPrefix,
                                 base::NumberToString(max_length + 1),
                                 kLongNumberRegexSuffix});
  re2::RE2 long_number_regex(regex_str);
  return re2::RE2::PartialMatch(str, long_number_regex);
}

bool ContainsForbiddenKeywords(const GURL& url) {
  auto path_and_query =
      base::StrCat({url.path_piece(), "?", url.query_piece()});
  for (const auto& regex_str : kPathAndQueryStringCheckRegexes) {
    re2::RE2 regex(regex_str);
    if (re2::RE2::PartialMatch(path_and_query, regex)) {
      return true;
    }
  }
  for (const auto& regex_str : kQueryStringAndRefCheckRegexes) {
    re2::RE2 regex(regex_str);
    if (!url.ref_piece().empty() &&
        re2::RE2::PartialMatch("#" + url.ref(), regex)) {
      return true;
    }
    if (!url.query_piece().empty() &&
        re2::RE2::PartialMatch("?" + url.query(), regex)) {
      return true;
    }
  }
  return false;
}

bool IsPrivateDomainLikely(const std::string_view host) {
  auto dot_split =
      base::SplitString(host, ".", base::WhitespaceHandling::KEEP_WHITESPACE,
                        base::SPLIT_WANT_ALL);
  if (dot_split.size() > kMaxDotSplitDomainSize) {
    return true;
  }
  if (CheckForLongNumber(host, kMaxDomainNumberLength)) {
    return true;
  }
  auto hyphen_split =
      base::SplitString(host, ".", base::WhitespaceHandling::KEEP_WHITESPACE,
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
  if (url.has_port() && url.port_piece() != "443") {
    VLOG(1) << "Ignoring URL due to non-standard port";
    return true;
  }
  if (matching_url_details && matching_url_details->is_search_engine) {
    if (url.has_ref() && url.ref_piece().length() > kMaxSearchEngineRefLength) {
      VLOG(1) << "Ignoring search engine URL due to long ref";
      return true;
    }
  }
  auto host_piece = url.host_piece();
  if (host_piece.ends_with(kOnionSiteSuffix) ||
      host_piece.ends_with(kLocalDomainSuffix) || host_piece == kLocalhost) {
    VLOG(1) << "Ignoring URL due a local host or onion site";
    return true;
  }
  if (IsPrivateDomainLikely(url.host_piece())) {
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
  if (CheckForLongNumber(query, kMaxQueryNumberLength)) {
    VLOG(1) << "Ignoring query due to long number";
    return true;
  }
  if (CheckForEmail(query)) {
    VLOG(1) << "Ignoring query due to inclusion of email";
    return true;
  }
  re2::RE2 http_pwd_regex(kHttpPasswordRegex);
  if (re2::RE2::PartialMatch(query, http_pwd_regex)) {
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

bool ShouldDropLongURL(const GURL& url) {
  if (CheckForEmail(url.spec())) {
    return true;
  }
  if (!url.query_piece().empty()) {
    if (url.query_piece().size() > kMaxQueryStringLength) {
      return true;
    }
    auto query_parts = base::SplitString(
        url.query_piece(), "&;", base::WhitespaceHandling::KEEP_WHITESPACE,
        base::SplitResult::SPLIT_WANT_ALL);
    if (query_parts.size() > kMaxQueryStringParts) {
      return true;
    }
    if (CheckForLongNumber(url.query_piece(),
                           kMaxQueryStringOrPathNumberLength)) {
      return true;
    }
  }
  if (!url.path_piece().empty()) {
    if (CheckForLongNumber(url.path_piece(),
                           kMaxQueryStringOrPathNumberLength)) {
      return true;
    }
  }
  auto path_parts = base::SplitString(url.path_piece(), "/._ -:+;",
                                      base::WhitespaceHandling::KEEP_WHITESPACE,
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
  auto path_segments = base::SplitString(
      url.path_piece(), "/", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
  for (const auto& path_segment : path_segments) {
    std::string alphanumeric_path_segment = path_segment;
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

  if (!ShouldDropLongURL(url)) {
    return url.spec();
  }

  if (url.host_piece().find(kGoogleHostSubstring) != std::string::npos &&
      url.has_query()) {
    auto google_url_param =
        ExtractValueFromQueryString(url.query_piece(), kGoogleURLQueryParam);
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
