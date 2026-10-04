/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_util.h"

#include <utility>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "third_party/re2/src/re2/re2.h"

namespace de_amp {

namespace {
// Check for "amp" or "⚡" in <html> tag
// https://amp.dev/documentation/guides-and-tutorials/learn/spec/amphtml/?format=websites#ampd
constexpr char kGetHtmlTagPattern[] = "(<\\s*?html\\s.*?>)";
// To see the expected behaviour of this regex, please see unit tests in
// de_amp_util_unittest.cc
constexpr char kDetectAmpPattern[] =
    "(?:<.*?\\s.*?(amp|⚡|⚡=\"(?:true|\\s*)\"|⚡=\'(?:true|\\s*)\'|amp=\"(?:"
    "true|\\s*)\"|amp='(?:true|\\s*)')(?:\\s.*?>|>|/>))";
// Look for canonical link tag and get href
// https://amp.dev/documentation/guides-and-tutorials/learn/spec/amphtml/?format=websites#canon
constexpr char kFindCanonicalLinkTagPattern[] =
    "(<\\s*?link\\s[^>]*?rel=(?:\"|')?canonical(?:\"|')?(?:\\s[^>]*?>|>|/>))";
constexpr char kFindCanonicalHrefInTagPattern[] =
    "href=(?:\"|')?(.*?)(?:\"|')?(?:\\s[^>]*?>|>|/>)";

RE2::Options InitRegexOptions() {
  RE2::Options opt;
  opt.set_case_sensitive(false);
  opt.set_dot_nl(true);
  return opt;
}

}  // namespace

bool IsDeAmpEnabled(PrefService* prefs) {
  return base::FeatureList::IsEnabled(features::kBraveDeAMP) &&
         prefs->GetBoolean(de_amp::kDeAmpPrefEnabled);
}

bool VerifyCanonicalAmpUrl(const GURL& canonical_link,
                           const GURL& original_url) {
  // Canonical URL should be a valid URL,
  // be HTTP(S) and not be the same as original URL
  return canonical_link.is_valid() && canonical_link.SchemeIsHTTPOrHTTPS() &&
         canonical_link != original_url;
}

bool CheckIfAmpPage(std::string_view body) {
  auto opt = InitRegexOptions();
  // The order of running these regexes is important:
  // we first get the relevant HTML tag and then find the info.
  static const base::NoDestructor<re2::RE2> kGetHtmlTagRegex(kGetHtmlTagPattern,
                                                             opt);
  static const base::NoDestructor<re2::RE2> kDetectAmpRegex(kDetectAmpPattern,
                                                            opt);

  std::string html_tag;
  if (!RE2::PartialMatch(body, *kGetHtmlTagRegex, &html_tag)) {
    // Early exit if we can't find HTML tag - malformed document (or error)
    return false;
  }
  if (!RE2::PartialMatch(html_tag, *kDetectAmpRegex)) {
    // Not AMP
    return false;
  }
  return true;
}

base::expected<std::string, std::string> FindCanonicalAmpUrl(
    std::string_view body) {
  auto opt = InitRegexOptions();
  // The order of running these regexes is important
  static const base::NoDestructor<re2::RE2> kFindCanonicalLinkTagRegex(
      kFindCanonicalLinkTagPattern, opt);
  static const base::NoDestructor<re2::RE2> kFindCanonicalHrefInTagRegex(
      kFindCanonicalHrefInTagPattern, opt);

  std::string link_tag;
  if (!RE2::PartialMatch(body, *kFindCanonicalLinkTagRegex, &link_tag)) {
    // Can't find link tag, exit
    return base::unexpected("Couldn't find link tag");
  }
  std::string canonical_url;
  // Find href in canonical link tag
  // Check there is only 1 href captured, else fail
  if (!RE2::PartialMatch(link_tag, *kFindCanonicalHrefInTagRegex,
                         &canonical_url)) {
    // Didn't find canonical link, potentially try again
    return base::unexpected("Couldn't find canonical URL in link tag");
  }
  return base::ok(std::move(canonical_url));
}

}  // namespace de_amp
