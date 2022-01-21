/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_service.h"

#include <string>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/de_amp/browser/de_amp_pref_names.h"
#include "brave/components/de_amp/common/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "third_party/re2/src/re2/re2.h"

namespace de_amp {

// Check for "amp" or "⚡" in <html> tag
// https://amp.dev/documentation/guides-and-tutorials/learn/spec/amphtml/?format=websites#ampd
static const char kDetectAmpPattern[] = "(?:<.*html\\s.*(amp|⚡)\\s.*>)";
// Look for canonical link
// https://amp.dev/documentation/guides-and-tutorials/learn/spec/amphtml/?format=websites#canon
static const char kFindCanonicalLinkPattern[] =
    "<.*link\\s.*rel=\"canonical\"\\s.*href=\"(.*?)\"";

DeAmpService::DeAmpService(PrefService* prefs) : prefs_(prefs) {}

DeAmpService::~DeAmpService() {}

// static
void DeAmpService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kDeAmpPrefEnabled, false);
}

void DeAmpService::ToggleDeAmp() {
  const bool enabled = prefs_->GetBoolean(kDeAmpPrefEnabled);
  prefs_->SetBoolean(kDeAmpPrefEnabled, !enabled);
}

void DeAmpService::DisableDeAmpForTest() {
  prefs_->SetBoolean(kDeAmpPrefEnabled, false);
}

bool DeAmpService::IsEnabled() {
  if (!base::FeatureList::IsEnabled(de_amp::features::kBraveDeAMP)) {
    return false;
  }

  const bool enabled = prefs_->GetBoolean(kDeAmpPrefEnabled);
  return enabled;
}

bool DeAmpService::CheckCanonicalLink(GURL canonical_link) {
  return canonical_link.SchemeIsHTTPOrHTTPS();
}

// If AMP page, find canonical link
bool DeAmpService::FindCanonicalLinkIfAMP(std::string body,
                                          std::string* canonical_link) {
  RE2::Options opt;
  opt.set_case_sensitive(false);
  static const base::NoDestructor<re2::RE2> kDetectAmpRegex(kDetectAmpPattern,
                                                            opt);
  static const base::NoDestructor<re2::RE2> kFindCanonicalLinkRegex(
      kFindCanonicalLinkPattern, opt);

  return RE2::PartialMatch(body, *kDetectAmpRegex) &&
         RE2::PartialMatch(body, *kFindCanonicalLinkRegex, canonical_link);
}

}  // namespace de_amp
