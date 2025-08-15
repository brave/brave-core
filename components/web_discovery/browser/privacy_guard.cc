/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/privacy_guard.h"

#include <algorithm>

#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/web_discovery/browser/hash_detection.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "brave/components/web_discovery/browser/util.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace web_discovery {

namespace {

constexpr size_t kMaxSearchEngineRefLength = 8;
constexpr size_t kMaxQueryLength = 120;
constexpr size_t kMaxQueryLengthWithLogograms = 50;
constexpr size_t kMaxQueryNumberLength = 7;
constexpr size_t kMinQueryWordsForCheck = 9;
constexpr size_t kMaxLongWords = 16;
constexpr size_t kMinLongWordLength = 4;
constexpr size_t kMaxWordLength = 45;
constexpr size_t kMinWordLengthForEuroCheck = 20;

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

bool HasLogograms(std::u16string_view str) {
  for (auto ch : str) {
    // Chinese: Range of Unicode code points for common Chinese characters
    if (ch >= 0x4e00 && ch <= 0x9fff) {
      return true;
    }
    // Japanese: Range of Unicode code points for Hiragana and Katakana
    // characters
    if (ch >= 0x3040 && ch <= 0x30ff) {
      return true;
    }
    // Korean: Range of Unicode code points for Hangul syllables
    if (ch >= 0xac00 && ch <= 0xd7af) {
      return true;
    }
    // Thai: Range of Unicode code points for Thai characters
    if (ch >= 0x0e00 && ch <= 0x0e7f) {
      return true;
    }
  }
  return false;
}

bool CheckWordLimits(const std::vector<std::u16string>& words,
                     std::u16string_view query) {
  // Check word count limits first
  if (words.size() > kMinQueryWordsForCheck) {
    size_t long_word_count = std::ranges::count_if(words, [](const auto& word) {
      return word.length() >= kMinLongWordLength;
    });

    if (long_word_count > kMaxLongWords) {
      VLOG(1) << "Ignoring query due to too many long words";
      return true;
    }
    if (HasLogograms(query)) {
      VLOG(1)
          << "Ignoring query due to too many words for query with logograms";
      return true;
    }
  }

  // Check individual word length limits
  for (const auto& word : words) {
    if (word.length() > kMaxWordLength) {
      VLOG(1) << "Ignoring query due to word that exceeds the max length";
      return true;
    }
    if (word.length() > kMinWordLengthForEuroCheck) {
      auto utf8_word = base::UTF16ToUTF8(word);
      // European long words are allowed, but other long words are not
      if (!RegexUtil::GetInstance()->CheckForEuroLongWord(utf8_word)) {
        VLOG(1) << "Ignoring query due to found long word (smaller limit but "
                   "uncommon shape)";
        return true;
      }
    }
  }
  return false;
}

bool ContainsForbiddenKeywords(const GURL& url) {
  auto path_and_query =
      base::StrCat({url.path_piece(), "?", url.query_piece()});
  if (RegexUtil::GetInstance()->CheckPathAndQueryStringKeywords(
          path_and_query)) {
    return true;
  }
  if (!url.ref_piece().empty() &&
      RegexUtil::GetInstance()->CheckQueryStringOrRefKeywords("#" +
                                                              url.ref())) {
    return true;
  }
  if (!url.query_piece().empty() &&
      RegexUtil::GetInstance()->CheckQueryStringOrRefKeywords("?" +
                                                              url.query())) {
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

bool IsPrivateURLLikely(const GURL& url, bool is_search_engine) {
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
  if (is_search_engine) {
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
  // First, normalize white spaces
  auto normalized_query = RegexUtil::GetInstance()->NormalizeWhitespace(query);

  // Convert to UTF-16 once for all length checks and logogram detection
  auto u16_query = base::UTF8ToUTF16(normalized_query);

  if (u16_query.length() > kMaxQueryLength) {
    VLOG(1) << "Ignoring query due to long length";
    return true;
  }

  if (u16_query.length() > kMaxQueryLengthWithLogograms &&
      HasLogograms(u16_query)) {
    VLOG(1) << "Ignoring query due to long length with logograms present";
    return true;
  }

  auto words = base::SplitString(u16_query, u" ",
                                 base::WhitespaceHandling::KEEP_WHITESPACE,
                                 base::SPLIT_WANT_NONEMPTY);

  if (CheckWordLimits(words, u16_query)) {
    return true;
  }

  if (RegexUtil::GetInstance()->CheckForLongNumber(normalized_query,
                                                   kMaxQueryNumberLength)) {
    VLOG(1) << "Ignoring query due to long number";
    return true;
  }
  if (RegexUtil::GetInstance()->CheckForEmail(normalized_query)) {
    VLOG(1) << "Ignoring query due to inclusion of email";
    return true;
  }
  if (RegexUtil::GetInstance()->CheckQueryHTTPCredentials(normalized_query)) {
    VLOG(1) << "Ignoring query due to potential inclusion of HTTP credentials";
    return true;
  }

  return false;
}

GURL GeneratePrivateSearchURL(const GURL& original_url,
                              const std::string& query,
                              std::optional<std::string_view> prefix) {
  url::RawCanonOutputT<char> query_encoded;
  url::EncodeURIComponent(query, &query_encoded);
  std::string query_encoded_str(query_encoded.view());
  base::ReplaceSubstringsAfterOffset(&query_encoded_str, 0, "%20", "+");

  return GURL(
      base::StrCat({original_url.scheme(), url::kStandardSchemeSeparator,
                    original_url.host(), "/",
                    prefix.value_or(kDefaultSearchPrefix), query_encoded_str}));
}

bool ShouldMaskURL(const GURL& url, bool relaxed) {
  if (RegexUtil::GetInstance()->CheckForEmail(url.spec())) {
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
    if (RegexUtil::GetInstance()->CheckForLongNumber(
            url.query_piece(), kMaxQueryStringOrPathNumberLength)) {
      return true;
    }
  }
  if (!url.path_piece().empty()) {
    if (RegexUtil::GetInstance()->CheckForLongNumber(
            url.path_piece(), kMaxQueryStringOrPathNumberLength)) {
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
    if (!relaxed && path_part.length() >= kMinPathPartHashCheckLength &&
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
    if (!relaxed &&
        alphanumeric_path_segment.length() >= kMinSegmentHashCheckLength &&
        IsHashLikely(alphanumeric_path_segment)) {
      return true;
    }
  }
  return ContainsForbiddenKeywords(url);
}

std::optional<std::string> MaskURL(const GURL& url, bool relaxed) {
  if (!url.SchemeIsHTTPOrHTTPS() || !url.is_valid()) {
    return std::nullopt;
  }

  if (!ShouldMaskURL(url, relaxed)) {
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
      return MaskURL(decoded_embedded_url, relaxed);
    }
  }

  return base::StrCat({url.scheme(), url::kStandardSchemeSeparator, url.host(),
                       kMaskedURLSuffix});
}

}  // namespace web_discovery
