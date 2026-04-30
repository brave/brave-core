/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define GetVariationsList GetVariationsList_ChromiumImpl
#include "base/strings/string_util.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"

#include <components/webui/version/version_handler_helper.cc>
#undef GetVariationsList

namespace version_ui {

namespace {

#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// Trial-name prefixes for studies that target features which are stripped from
// Brave Origin builds at compile time (see is_brave_origin_branded gating in
// //brave/components/*/buildflags). The variations seed is shared across all
// Brave builds, so these studies still register field-trial groups, but the
// underlying features are not compiled in and the studies have no effect.
// Filter them from chrome://version so the page reflects what actually ships.
constexpr std::string_view kBraveOriginInapplicableTrialPrefixes[] = {
    "AIChat",                    // enable_ai_chat
    "BraveAds",                  // enable_brave_ads
    "BraveNTPBrandedWallpaper",  // enable_brave_ads (NTP sponsored images)
    "BraveNTPSponsored",         // enable_brave_ads (NTP sponsored images)
    "BraveNews",                 // enable_brave_news
    "BraveRewards",              // enable_brave_rewards
    "BraveTalk",                 // enable_brave_talk
    "BraveVPN",                  // enable_brave_vpn
    "BraveVpn",                  // enable_brave_vpn (alternate casing)
    "BraveWallet",               // enable_brave_wallet
    "BraveWaybackMachine",       // enable_brave_wayback_machine
    "BraveWebDiscovery",         // enable_web_discovery
    "EmailAliases",              // enable_email_aliases
    "Leo",                       // alias for AI Chat
    "Playlist",                  // enable_playlist
    "Psst",                      // enable_psst
    "Speedreader",               // enable_speedreader
    "Tor",                       // enable_tor
    "WebDiscovery",              // enable_web_discovery
    "ZCash",                     // enable_brave_wallet (ZCash sub-feature)
};

bool IsBraveOriginInapplicableTrial(const std::string& trial_name) {
  for (std::string_view prefix : kBraveOriginInapplicableTrialPrefixes) {
    if (base::StartsWith(trial_name, prefix)) {
      return true;
    }
  }
  return false;
}
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)

}  // namespace

// Brave always shows full variations names instead of hashes.
base::ListValue GetVariationsList() {
  std::vector<std::string> variations;
  base::FieldTrial::ActiveGroups active_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_groups);

  const unsigned char kNonBreakingHyphenUTF8[] = {0xE2, 0x80, 0x91, '\0'};
  const std::string kNonBreakingHyphenUTF8String(
      reinterpret_cast<const char*>(kNonBreakingHyphenUTF8));
  for (const auto& group : active_groups) {
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
    if (IsBraveOriginInapplicableTrial(group.trial_name)) {
      continue;
    }
#endif
    std::string line = group.trial_name + ":" + group.group_name;
    base::ReplaceChars(line, "-", kNonBreakingHyphenUTF8String, &line);
    variations.push_back(line);
  }

  base::ListValue variations_list;
  const std::string& seed_version = variations::GetSeedVersion();
  if (!seed_version.empty() && seed_version != "1") {
    variations_list.Append(seed_version);
  }
  for (std::string& variation : variations) {
    variations_list.Append(std::move(variation));
  }

  return variations_list;
}

}  // namespace version_ui
