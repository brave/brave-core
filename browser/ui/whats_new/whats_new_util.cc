/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/whats_new/whats_new_util.h"

#include <optional>
#include <string>

#include "base/metrics/field_trial_params.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/version.h"
#include "brave/browser/ui/whats_new/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "chrome/browser/profiles/chrome_version_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "url/gurl.h"

using version_info::Channel;

namespace {

double g_testing_major_version = 0;

// |version| has four components like 111.1.51.34.
// First one is upstream's major version.
// Brave's major version is second and third component like 1.51.
// Ignored fourth number as it's build number.
std::optional<double> GetBraveMajorVersionAsDouble(
    const base::Version& version) {
  double brave_major_version;
  if (!base::StringToDouble(base::StringPrintf("%d.%d", version.components()[1],
                                               version.components()[2]),
                            &brave_major_version)) {
    return std::nullopt;
  }

  return brave_major_version;
}

// Returns 1.xx or 2.xx as double.
std::optional<double> GetCurrentBrowserVersion() {
  if (g_testing_major_version != 0) {
    return g_testing_major_version;
  }

  const auto& version = version_info::GetVersion();
  DCHECK(version.IsValid());
  DCHECK_EQ(version.components().size(), 4ul);

  return GetBraveMajorVersionAsDouble(version);
}

bool DoesUserGetMajorUpdateSinceInstall() {
  Profile* profile = ProfileManager::GetLastUsedProfileIfLoaded();

  // This could happen when selected profile from profile chooser dialog is not
  // the last active profile from previous running. As we don't know
  // profile_created_version for this selected profile now, just return false
  // and whats-new tab will not be shown for this launching. but that value
  // could be get next time when this profile is selected again. This early
  // return could be happened forever when user select another profile from
  // profile chooser whenver launching. but I think this could be very very rare
  // case. So, user could show whats-new tab eventually. Also this doesn't
  // happen when profile chooser is not used even user has multiple profiles.
  if (!profile) {
    return false;
  }

  const auto current_version = GetCurrentBrowserVersion();
  const auto profile_created_version = GetBraveMajorVersionAsDouble(
      base::Version(ChromeVersionService::GetVersion(profile->GetPrefs())));
  if (!current_version || !profile_created_version) {
    return false;
  }

  VLOG(2) << __func__ << " : current_version: " << *current_version
          << ", profile_created_version: " << *profile_created_version;
  // If profile created version and current version is different,
  // we think this user had major version update since the install.
  return current_version != profile_created_version;
}

std::optional<double> GetTargetMajorVersion() {
  constexpr char kWhatsNewTrial[] = "WhatsNewStudy";

  const std::string target_major_version_string = base::GetFieldTrialParamValue(
      kWhatsNewTrial, whats_new::GetTargetMajorVersionParamName());
  // Field trial doesn't have this value.
  if (target_major_version_string.empty()) {
    return std::nullopt;
  }

  double target_major_version;
  if (!base::StringToDouble(target_major_version_string,
                            &target_major_version)) {
    return std::nullopt;
  }

  return target_major_version;
}

}  // namespace

namespace whats_new {

std::string GetTargetMajorVersionParamName() {
  switch (chrome::GetChannel()) {
    case Channel::STABLE:
      return "target_major_version_stable";
    case Channel::BETA:
      return "target_major_version_beta";
    case Channel::DEV:
      return "target_major_version_dev";
    case Channel::CANARY:
      return "target_major_version_nightly";
    case Channel::UNKNOWN:
      return "target_major_version_unknown";
  }
  NOTREACHED_NORETURN();
}

void SetCurrentVersionForTesting(double major_version) {
  g_testing_major_version = major_version;
}

bool ShouldShowBraveWhatsNewForState(PrefService* local_state) {
  if (!DoesUserGetMajorUpdateSinceInstall()) {
    VLOG(2) << __func__ << " : This user doesn't get major update yet.";
    return false;
  }

  // Supported languages/translations for the What's New page are:
  // Simplified Chinese, French, German, Japanese, Korean, Portuguese, and
  // Spanish.
  constexpr std::array<const char*, 8> kSupportedLanguages = {
      "en", "zh", "fr", "de", "ja", "ko", "pt", "es"};
  const std::string default_lang_code =
      brave_l10n::GetDefaultISOLanguageCodeString();
  if (base::ranges::find(kSupportedLanguages, default_lang_code) ==
      std::end(kSupportedLanguages)) {
    VLOG(2) << __func__ << " Not supported language - " << default_lang_code;
    return false;
  }

  auto target_major_version = GetTargetMajorVersion();
  // false if whatsnew is not supported in this country.
  if (!target_major_version) {
    VLOG(2) << __func__ << " Field trial doesn't have target_major_version";
    return false;
  }

  const auto current_version = GetCurrentBrowserVersion();
  if (!current_version) {
    NOTREACHED_IN_MIGRATION() << __func__ << " Should get current version.";
    return false;
  }

  if (*current_version != *target_major_version) {
    VLOG(2) << __func__ << " Current version is different with target version";
    VLOG(2) << __func__ << " Current version - " << *current_version;
    VLOG(2) << __func__ << " Target version - " << *target_major_version;
    return false;
  }

  // Already shown whatsnew.
  const double last_version =
      local_state->GetDouble(prefs::kWhatsNewLastVersion);
  if (last_version == *target_major_version) {
    VLOG(2) << __func__ << " Already shown for " << *target_major_version;
    return false;
  }

  // Set the last version here to indicate that What's New should not attempt
  // to display again for this milestone. This prevents the page from
  // potentially displaying multiple times in a given milestone, e.g. for
  // multiple profile relaunches (see https://crbug.com/1274313).
  local_state->SetDouble(prefs::kWhatsNewLastVersion, *target_major_version);
  return true;
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDoublePref(prefs::kWhatsNewLastVersion, 0);
}

void StartBraveWhatsNew(Browser* browser) {
  constexpr char kBraveWhatsNewURL[] = "https://brave.com/whats-new/";
  // Load whats-new url in the first foreground tab.
  chrome::AddTabAt(browser, GURL(kBraveWhatsNewURL), 0, true);
  browser->tab_strip_model()->ActivateTabAt(
      browser->tab_strip_model()->IndexOfFirstNonPinnedTab());
}

}  // namespace whats_new
