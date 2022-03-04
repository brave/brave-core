/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_service.h"

#include <string>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "third_party/re2/src/re2/re2.h"

namespace de_amp {

// Check for "amp" or "⚡" in <html> tag
// https://amp.dev/documentation/guides-and-tutorials/learn/spec/amphtml/?format=websites#ampd
static const char kGetHtmlTagPattern[] = "(<\\s*?html\\s.*?>)";
static const char kDetectAmpPattern[] = "(?:<.*\\s.*(amp|⚡)(?:\\s.*>|>|/>))";
// Look for canonical link tag and get href
// https://amp.dev/documentation/guides-and-tutorials/learn/spec/amphtml/?format=websites#canon
static const char kFindCanonicalLinkTagPattern[] =
    "(<\\s*link\\s[^>]*rel=(?:\"|')canonical(?:\"|')(?:\\s[^>]*>|>|/>))";
static const char kFindCanonicalHrefInTagPattern[] =
    "href=(?:\"|')(.*?)(?:\"|')";

DeAmpService::DeAmpService(PrefService* prefs) : prefs_(prefs) {}

DeAmpService::~DeAmpService() {}

// static
void DeAmpService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kDeAmpPrefEnabled, true); // default on
}

void DeAmpService::ToggleDeAmp(const bool on) {
  prefs_->SetBoolean(kDeAmpPrefEnabled, on);
}

bool DeAmpService::IsEnabled() {
  if (!base::FeatureList::IsEnabled(de_amp::features::kBraveDeAMP)) {
    return false;
  }

  return prefs_->GetBoolean(kDeAmpPrefEnabled);
}

// static
bool DeAmpService::VerifyCanonicalLink(const GURL canonical_link,
                                       const GURL original_url) {
  // Canonical URL should be a valid URL,
  // be HTTP(S) and not be the same as original URL
  return canonical_link.is_valid() && canonical_link.SchemeIsHTTPOrHTTPS() &&
         canonical_link != original_url;
}

// If AMP page, find canonical link
// canonical link param is populated if found
// static
bool DeAmpService::FindCanonicalLinkIfAMP(const std::string& body,
                                          std::string* canonical_link) {
  RE2::Options opt;
  opt.set_case_sensitive(false);
  opt.set_dot_nl(true);
  // The order of running these regexes is important:
  // we first get the relevant HTML tag and then find the info.
  static const base::NoDestructor<re2::RE2> kGetHtmlTagRegex(kGetHtmlTagPattern,
                                                             opt);
  static const base::NoDestructor<re2::RE2> kDetectAmpRegex(kDetectAmpPattern,
                                                            opt);
  static const base::NoDestructor<re2::RE2> kFindCanonicalLinkTagRegex(
      kFindCanonicalLinkTagPattern, opt);
  static const base::NoDestructor<re2::RE2> kFindCanonicalHrefInTagRegex(
      kFindCanonicalHrefInTagPattern, opt);

  std::string html_tag;
  if (!RE2::PartialMatch(body, *kGetHtmlTagRegex, &html_tag)) {
    // Early exit if we can't find HTML tag - malformed document (or error)
    return false;
  }
  if (!RE2::PartialMatch(html_tag, *kDetectAmpRegex)) {
    // Not AMP
    return false;
  }
  std::string link_tag;
  if (!RE2::PartialMatch(body, *kFindCanonicalLinkTagRegex, &link_tag)) {
    // Can't find link tag, exit
    return false;
  }
  // Find href in canonical link tag
  return RE2::PartialMatch(link_tag, *kFindCanonicalHrefInTagRegex,
                           canonical_link);
}

}  // namespace de_amp
