// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/search_suggestions/query_check_utils.h"

#include <cmath>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "brave/components/omnibox/browser/search_suggestions/query_check_constants.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace {

size_t GetPosForHashChars(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }

  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 10;
  }

  if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 10 + 26;
  }

  NOTREACHED_NORETURN();
}

double GetHashProb(const std::string& query) {
  double log_prob = 0.0;
  int trans_c = 0;

  CHECK(query.length() >= 2);
  for (size_t i = 0; i < query.length() - 1; i++) {
    const auto a = query[i];
    const auto b = query[i + 1];
    const auto pos1 = GetPosForHashChars(a);
    const auto pos2 = GetPosForHashChars(b);
    log_prob += UNSAFE_TODO(kProbHashLogM[pos1][pos2]);
    trans_c += 1;
  }

  if (trans_c > 0) {
    return exp(log_prob / trans_c);
  }

  return exp(log_prob);
}

bool LooksLikeHttpPass(const std::string& query) {
  constexpr char pattern[] = "[^:]+:[^@]+@";
  if (RE2::PartialMatch(query, pattern)) {
    return true;
  }

  const std::vector<std::string_view> tokens = base::SplitStringPiece(
      query, " ", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);
  for (const auto& token : tokens) {
    if (RE2::PartialMatch(token, pattern)) {
      return true;
    }
  }

  return false;
}

bool HasEmailInQuery(const std::string& query) {
  static const base::NoDestructor<RE2> regex(
      "[a-z0-9\\-_@]+(@|%40|%(25)+40)[a-z0-9\\-_]+\\.[a-z0-9\\-_]");
  return RE2::PartialMatch(query, *regex.get());
}

bool HasLongNumberInQuery(const std::string& query) {
  std::string updated_query = query;
  RE2::GlobalReplace(&updated_query, "[^A-Za-z0-9]", "");
  RE2::GlobalReplace(&updated_query, "[A-Za-z]+", " ");

  const std::vector<std::string_view> numbers = base::SplitStringPiece(
      updated_query, " ", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  constexpr int kMaxAllowedNumberLength = 7;
  for (const auto& number : numbers) {
    if (number.length() > kMaxAllowedNumberLength) {
      return true;
    }
  }

  return false;
}

bool HasValidWordCountInQuery(const std::string& query) {
  std::vector<std::string_view> words_in_query = base::SplitStringPiece(
      query, " ", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  constexpr int kMinWordCountInQueryForBlocking = 7;
  return words_in_query.size() <= kMinWordCountInQueryForBlocking;
}

bool CheckHashProb(const std::string& query) {
  if (query.length() <= 12) {
    return false;
  }

  std::string updated_query = query;
  RE2::GlobalReplace(&updated_query, "[^A-Za-z0-9]", "");
  if (updated_query.length() <= 12) {
    return false;
  }

  const double pp = GetHashProb(updated_query);
  // we are a bit more strict here because the query
  // can have parts well formed
  if (pp < kProbHashThreshold * 1.5) {
    return true;
  }

  return false;
}

// Warning: This function is very specialized. Do not use it outside
// this module. It will misclassify shortener links like
// "http://tinyurl.com/oqnffw3" because the host name is too big.
//
// Precondition: query is very small.
//
// It is the last safety net to avoid sending URLs from shortener services
// to the search because they do not exceed the minimum size
// (e.g., "is.gd/PazNcR", "t.co/RUiFUYKzkz").
//
bool IsSmallQueryButCouldBeUrlShortener(const std::string& query) {
  size_t pos = query.find('/');
  if (pos == std::string::npos) {
    return false;
  }

  const auto host = query.substr(0, pos);
  const auto rest = query.substr(pos + 1);
  const std::vector<std::string_view> host_tokens = base::SplitStringPiece(
      host, ".", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);
  return !rest.empty() && rest.length() >= 4 && host.length() <= 7 &&
         host_tokens.size() == 2;
}

// WARNING: This function is not a proper URL parser and it should not be used
// for any other purposes.
GURL TryParseAsUrlWithIncompleteSchema(const std::string& query) {
  // Relying on the constructor "URL" alone to detect valid URLs is
  // problematic, as it tries very hard to parse any string.
  // For instance, depending on the browser's URL implementation
  // "http://bayern münchen" will be seen as a valid URL, but we do
  // not want to block the query "bayern münchen".
  //
  // In addition, stop guessing if the query is quoted. If it is
  // an exact URL (with "http[s]://"), it has been already handled.
  const auto has_valid_host_name = [](const std::string& query) {
    if (RE2::PartialMatch(query, "\\s")) {
      return false;
    }
    const std::vector<std::string_view> tokens = base::SplitStringPiece(
        query, "/", base::WhitespaceHandling::TRIM_WHITESPACE,
        base::SplitResult::SPLIT_WANT_NONEMPTY);
    if (tokens.empty()) {
      return false;
    }
    const auto host = tokens[0];
    return !RE2::PartialMatch(host, "['\"]");
  };

  const auto try_parse = [&has_valid_host_name](const std::string& query) {
    if (has_valid_host_name(query)) {
      const auto url = GURL(base::StrCat({"http://", query}));
      if (url.is_valid()) {
        return url;
      }
    }
    return GURL();
  };

  GURL url = try_parse(query);
  if (url.is_valid()) {
    return url;
  }

  // no valid URL, try again but first remove relicts of the
  // schema, for example, if someone is deleting characters
  // from the start of the URL.
  std::string trunc_query = query;
  RE2::GlobalReplace(&trunc_query, "^['\"]?h?t?t?p?s?:?\\/\\/", "");
  if (trunc_query.length() != query.length()) {
    url = try_parse(trunc_query);
    if (url.is_valid()) {
      return url;
    }
  }

  return GURL();
}

// Very crude heuristic to protect against leaking urls.
// Assumes that real URLs, starting with "http[s]://"
// have been already filtered. The rough idea is to
// try whether "http://<query>" is a valid URL and whether
// it has enough sensitive information to block the search.
bool IsPotentiallyLeakingUrlInformation(const std::string& query) {
  // Early exit: If the URL is too small, we can avoid the
  // more expensive checks. This value should be quite conservative,
  // otherwise URL shorter links may slip through
  // (e.g., "goo.gl/bdkh1L", "t.co/RUiFUYKzkz", 'is.gd/PazNcR').
  //
  // Note: URL shorteners in general are a problem, as they provide
  // an extremely compact representation of an URL. Although it is
  // safe to assume that they do not encode URLs with secrets, we
  // would still leak the site that the user is going to visit.
  if (query.length() <= 11 ||
      (query.length() <= 18 && !IsSmallQueryButCouldBeUrlShortener(query))) {
    return false;
  }

  const auto url = TryParseAsUrlWithIncompleteSchema(query);
  if (!url.is_valid()) {
    // does not look like an URL --> safe
    return false;
  }

  // reject non-public URLs
  if (url.has_username() || url.has_password() || url.has_port()) {
    return true;
  }

  // If the URL path is non empty, it is a strong indicator
  // that the user is currently typing an URL:
  //
  // * If the path name itself gets too long, we have to be careful
  //   because of links from URL shortener (e.g., 'bit.ly/1h0ceQI').
  // * If it contains URL search paramters in addition to the
  //   path ('<domain>/path?param[=key]'), also stop.
  //
  // Note: ".search" without ".pathname" is quite aggressive,
  // for instance, 'Fu?ball' (misspelling for "Fußball"), would
  // already match ("http://Fu?ball" => host: "Fu", search: "ball")
  if (url.path() != "/" && (url.path().length() >= 6 || url.has_query())) {
    return true;
  }

  const auto domain_guessable = RE2::PartialMatch(query, "\\w+[.]\\w+\\/");
  if (domain_guessable) {
    return true;
  }

  // looks safe
  return false;
}

}  // namespace

namespace search_suggestions {

bool IsSuspiciousQuery(const std::string& query) {
  if (!HasValidWordCountInQuery(query)) {
    return true;
  }

  if (HasLongNumberInQuery(query)) {
    return true;
  }

  if (HasEmailInQuery(query)) {
    return true;
  }

  if (LooksLikeHttpPass(query)) {
    return true;
  }

  if (CheckHashProb(query)) {
    return true;
  }

  return false;
}

bool IsSafeQueryUrl(const std::string& query) {
  // fast path, which should handle most calls
  const auto is_small_enough = [](const std::string& query) {
    return query.length() <= 6;
  };

  if (is_small_enough(query)) {
    return true;
  }

  std::string updated_query = query;
  RE2::GlobalReplace(&updated_query, "\\s+", " ");
  if (is_small_enough(updated_query)) {
    return true;
  }

  // Do not attempt to search for long texts. Maybe the user
  // accidentally copied a sensitive email to the clipboard.
  //
  // However, if the limit is too low, we miss valid queries
  // when someone copies error messages and searches for it.
  if (updated_query.length() > 100) {
    return false;
  }

  if (IsPotentiallyLeakingUrlInformation(query)) {
    // This rule is vague, but there is enough evidence that the user
    // is currently editing an URL. Do not query the backend, but
    // instead rely on history information only.
    return false;
  }

  return true;
}

}  // namespace search_suggestions
