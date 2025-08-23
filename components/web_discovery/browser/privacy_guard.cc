/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/privacy_guard.h"

#include <algorithm>
#include <optional>

#include "base/containers/fixed_flat_set.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "brave/components/web_discovery/browser/util.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace web_discovery {

namespace {

constexpr size_t kMaxQueryLength = 120;
constexpr size_t kMaxQueryLengthWithLogograms = 50;
constexpr size_t kMinQueryWordsForCheck = 9;
constexpr size_t kMaxLongWords = 16;
constexpr size_t kMinLongWordLength = 4;
constexpr size_t kMaxWordLength = 45;
constexpr size_t kMinWordLengthForEuroCheck = 20;

constexpr size_t kMinNumberLengthToCheck = 3;
constexpr size_t kMaxNumberLength = 7;

constexpr size_t kMaxHostnameLength = 50;
constexpr size_t kMaxUrlLength = 800;
constexpr size_t kMaxUrlSearchLength = 150;
constexpr size_t kMaxUrlSearchParams = 8;
constexpr size_t kMaxUrlPathParts = 8;

constexpr auto kRiskyUrlPathParts = base::MakeFixedFlatSet<std::string_view>({
    // login related:
    "login",
    "login.php",
    "login-actions",
    "logout",
    "signin",
    "recover",
    "forgot",
    "forgot-password",
    "reset-credentials",
    "authenticate",
    "not-confirmed",
    "reset",
    "oauth",
    "password",
    // potential tokens
    "token",
    // could leak account:
    "edit",
    "checkout",
    "account",
    "share",
    "sharing",
    // Admin accounts
    "admin",
    "console",
    // Wordpress
    "wp-admin",
    "wp-admin.php",
    // Oracle WebLogic
    "weblogic",
});

constexpr char kDefaultSearchPrefix[] = "search?q=";
constexpr char kLocalhost[] = "localhost";
constexpr char kProtectedSuffix[] = " (PROTECTED)";

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

bool IsValidEAN13(std::string_view ean) {
  if (ean.length() != 13) {
    return false;
  }

  // Calculate checksum (digits already validated by caller)
  int sum = 0;
  for (size_t i = 0; i < 12; i++) {
    int factor = (i % 2 == 0) ? 1 : 3;
    sum += factor * (ean[i] - '0');
  }

  int checksum = (10 - (sum % 10)) % 10;
  return checksum == (ean[12] - '0');
}

std::optional<std::string> FindValidISSN(std::string_view str) {
  std::string issn_candidate;

  // Loop through all ISSN candidates until we find one with valid checksum
  while (RegexUtil::GetInstance()->FindAndConsumeISSN(&str, &issn_candidate)) {
    // Validate checksum
    int checksum = 0;
    size_t position = 0;

    for (char c : issn_candidate) {
      if (c == '-') {
        continue;
      }

      if (position == 7) {
        // This is the check digit (position 7, 8th character)
        checksum += (c == 'x' || c == 'X') ? 10 : (c - '0');
        break;  // We're done once we process the check digit
      } else {
        checksum += (c - '0') * (8 - position);
      }
      position++;
    }

    if (checksum % 11 == 0) {
      return issn_candidate;  // Found valid ISSN!
    }
  }

  return std::nullopt;  // No valid ISSN found
}

bool CheckForLongNumber(std::string str) {
  // Find and remove valid ISSN (as per reference implementation)
  auto valid_issn = FindValidISSN(str);
  if (valid_issn) {
    size_t pos = str.find(*valid_issn);
    if (pos != std::string::npos) {
      str.replace(pos, valid_issn->length(), " ");
    }
  }

  // Extract digit sequences using regex to find number fragments
  std::vector<std::string> numbers;
  std::string digits_only;
  std::string_view input = str;

  while (RegexUtil::GetInstance()->FindAndConsumeNumberFragment(&input,
                                                                &digits_only)) {
    // Add to numbers if long enough (filtering already done in
    // FindAndConsumeNumberFragment)
    if (digits_only.length() >= kMinNumberLengthToCheck) {
      numbers.push_back(digits_only);
    }
  }

  // Special handling for single 13-digit number (EAN-13 check)
  if (numbers.size() == 1 && numbers[0].length() == 13) {
    return !IsValidEAN13(numbers[0]);
  }

  // Check if any number exceeds max_length
  return std::ranges::any_of(
      numbers, [](const auto& num) { return num.length() > kMaxNumberLength; });
}

bool CheckPathAndQueryParts(const GURL& url,
                            const std::vector<std::string_view>& path_parts,
                            const std::vector<std::string_view>& query_parts) {
  // Check for risky path parts
  for (const auto& path_part : path_parts) {
    std::string normalized = base::ToLowerASCII(path_part);
    base::ReplaceChars(normalized, "_", "-", &normalized);

    if (kRiskyUrlPathParts.contains(normalized)) {
      return true;
    }
  }

  // Check URL parameters for suspicious content
  for (const auto& param : query_parts) {
    auto key_value = base::SplitStringPiece(
        param, "=", base::WhitespaceHandling::KEEP_WHITESPACE,
        base::SplitResult::SPLIT_WANT_ALL);
    if (key_value.size() != 2) {
      continue;
    }
    const auto& value = key_value[1];

    if (value.empty() ||
        RegexUtil::GetInstance()->CheckForSafeUrlParameter(value)) {
      continue;
    }
    // The value does not pass the first trivial check.
    // Use the private query checks to be more thorough.
    if (IsPrivateQueryLikely(value)) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool IsPrivateQueryLikely(std::string_view query) {
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

  if (CheckForLongNumber(normalized_query)) {
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
                              std::string_view query,
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

bool ShouldMaskURL(const GURL& url) {
  // Length checks
  if (url.spec().length() > kMaxUrlLength) {
    return true;
  }

  if (url.has_ref()) {
    return true;
  }

  if (RegexUtil::GetInstance()->CheckForMiscPrivateUrls(url.spec())) {
    return true;
  }

  std::vector<std::string_view> query_parts;
  if (!url.query_piece().empty()) {
    if (url.query_piece().length() > kMaxUrlSearchLength) {
      return true;
    }
    query_parts = base::SplitStringPiece(
        url.query_piece(), "&", base::WhitespaceHandling::KEEP_WHITESPACE,
        base::SplitResult::SPLIT_WANT_ALL);
    if (query_parts.size() > kMaxUrlSearchParams) {
      return true;
    }
  }

  auto decoded_url = DecodeURLComponent(url.spec());
  if (RegexUtil::GetInstance()->CheckForEmail(decoded_url) ||
      RegexUtil::GetInstance()->CheckForEmail(url.spec())) {
    return true;
  }

  // Check path parts count
  auto path_parts = base::SplitStringPiece(
      url.path_piece(), "/", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);
  if (path_parts.size() > kMaxUrlPathParts) {
    return true;
  }

  if (CheckPathAndQueryParts(url, path_parts, query_parts)) {
    return true;
  }

  return false;
}

std::optional<std::string> MaskURL(const GURL& url, bool relaxed) {
  // First check if URL should be dropped entirely
  if (ShouldDropURL(url)) {
    return std::nullopt;
  }

  // If URL doesn't need masking, return as-is
  if (!ShouldMaskURL(url)) {
    return url.spec();
  }

  // Try to preserve path if relaxed mode and it's safe
  if (relaxed && (!url.query_piece().empty() || !url.ref_piece().empty())) {
    GURL::Replacements replacements;
    replacements.ClearQuery();
    replacements.ClearRef();
    GURL url_without_query_and_ref = url.ReplaceComponents(replacements);

    auto path_only_result = MaskURL(url_without_query_and_ref, false);
    if (path_only_result) {
      if (base::EndsWith(*path_only_result, kProtectedSuffix)) {
        return *path_only_result;
      }
      return base::StrCat({*path_only_result, kProtectedSuffix});
    }
  }

  return base::StrCat({url.scheme(), url::kStandardSchemeSeparator, url.host(),
                       "/", kProtectedSuffix});
}

bool ShouldDropURL(const GURL& url) {
  if (!url.is_valid()) {
    return true;
  }

  if (url.has_username() || url.has_password()) {
    return true;
  }

  if (url.has_port() && url.port_piece() != "80" && url.port_piece() != "443") {
    return true;
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return true;
  }

  if (url.host_piece() == kLocalhost || url.HostIsIPAddress()) {
    return true;
  }

  if (url.host_piece().length() > kMaxHostnameLength) {
    return true;
  }

  return false;
}

}  // namespace web_discovery
