// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_CONSTANTS_H_

#include <cstdint>
#include <iterator>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/map_util.h"
#include "base/files/file_path.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "crypto/sha2.h"

namespace brave_shields {

// Content/Web settings:
inline constexpr char kAds[] = "shieldsAds";
inline constexpr char kCosmeticFiltering[] = "cosmeticFilteringV2";
inline constexpr char kTrackers[] = "trackers";
inline constexpr char kHTTPUpgradableResources[] = "httpUpgradableResources";
inline constexpr char kHTTPSUpgrades[] = "httpsUpgrades";
inline constexpr char kJavaScript[] = "javascript";
inline constexpr char kFingerprintingV2[] = "fingerprintingV2";
inline constexpr char kBraveShields[] = "braveShields";
inline constexpr char kBraveShieldsMetadata[] = "braveShieldsMetadata";
inline constexpr char kReferrers[] = "referrers";
inline constexpr char kCookies[] = "shieldsCookiesV3";
inline constexpr char kBraveAutoShred[] = "braveAutoShred";

// Prefs:
inline constexpr char kFacebookEmbeds[] = "fb-embeds";
inline constexpr char kTwitterEmbeds[] = "twitter-embeds";
inline constexpr char kLinkedInEmbeds[] = "linked-in-embeds";

inline constexpr auto kShieldsContentSettingsTypes =
    base::MakeFixedFlatSet<ContentSettingsType>({
        ContentSettingsType::BRAVE_ADS,
        ContentSettingsType::BRAVE_COSMETIC_FILTERING,
        ContentSettingsType::BRAVE_TRACKERS,
        ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
        ContentSettingsType::BRAVE_FINGERPRINTING_V2,
        ContentSettingsType::BRAVE_SHIELDS,
        ContentSettingsType::BRAVE_REFERRERS,
        ContentSettingsType::BRAVE_COOKIES,
        ContentSettingsType::BRAVE_AUTO_SHRED,
    });

using ShieldsContentSettingsTypes = decltype(kShieldsContentSettingsTypes);

inline constexpr auto kShieldsContentTypeNames =
    base::MakeFixedFlatMap<ContentSettingsType, const char*>({
        {ContentSettingsType::BRAVE_ADS, kAds},
        {ContentSettingsType::BRAVE_COSMETIC_FILTERING, kCosmeticFiltering},
        {ContentSettingsType::BRAVE_TRACKERS, kTrackers},
        {ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
         kHTTPUpgradableResources},
        {ContentSettingsType::BRAVE_HTTPS_UPGRADE, kHTTPSUpgrades},
        {ContentSettingsType::JAVASCRIPT, kJavaScript},
        {ContentSettingsType::BRAVE_FINGERPRINTING_V2, kFingerprintingV2},
        {ContentSettingsType::BRAVE_SHIELDS, kBraveShields},
        {ContentSettingsType::BRAVE_SHIELDS_METADATA, kBraveShieldsMetadata},
        {ContentSettingsType::BRAVE_REFERRERS, kReferrers},
        {ContentSettingsType::BRAVE_COOKIES, kCookies},
        {ContentSettingsType::BRAVE_AUTO_SHRED, kBraveAutoShred},
    });

using ShieldsContentTypeNames = decltype(kShieldsContentTypeNames);

namespace internal {
consteval bool CheckShieldsContentTypeNames() {
  for (const auto& [key_a, value_a] : kShieldsContentTypeNames) {
    for (const auto& [key_b, value_b] : kShieldsContentTypeNames) {
      if (key_a == key_b) {
        continue;
      }
      if (!value_a || !value_b || value_a == value_b) {
        // Name is null or not unique.
        return false;
      }
    }
  }
  return true;
}

static_assert(
    CheckShieldsContentTypeNames(),
    "Invalid kShieldsContentTypeNames. Name should be unique and non-null.");
}  // namespace internal

// Values used before the migration away from ResourceIdentifier, kept
// around for migration purposes only.
inline constexpr char kObsoleteAds[] = "ads";
inline constexpr char kObsoleteCookies[] = "cookies";
inline constexpr char kObsoleteShieldsCookies[] = "shieldsCookies";
inline constexpr char kObsoleteCosmeticFiltering[] = "cosmeticFiltering";

// Some users were not properly migrated from fingerprinting V1.
inline constexpr char kObsoleteFingerprinting[] = "fingerprinting";

// Key for procedural and action filters in the UrlCosmeticResources struct from
// adblock-rust
inline constexpr char kCosmeticResourcesProceduralActions[] =
    "procedural_actions";

// Filename for cached text from a custom filter list subscription
const base::FilePath::CharType kCustomSubscriptionListText[] =
    FILE_PATH_LITERAL("list_text.txt");

inline constexpr char kDefaultAdblockFiltersListUuid[] = "default";
inline constexpr char kFirstPartyAdblockFiltersListUuid[] =
    "E99CBD02-FFD1-4651-9BDD-6A9ED7B87819";
inline constexpr char kCookieListUuid[] =
    "AC023D22-AE88-4060-A978-4FEEEC4221693";
inline constexpr char kMobileNotificationsListUuid[] =
    "2F3DCE16-A19A-493C-A88F-2E110FBD37D6";
inline constexpr char kExperimentalListUuid[] =
    "564C3B75-8731-404C-AD7C-5683258BA0B0";
inline constexpr char kAdblockOnlySupplementalListUuid[] =
    "61C6046C-B040-4CE4-8C8E-F3E8F316AA72";

inline constexpr char kAdBlockResourceComponentName[] =
    "Brave Ad Block Resources Library";
inline constexpr char kAdBlockResourceComponentId[] =
    "mfddibmblmbccpadfndgakiopmmhebop";
inline constexpr uint8_t kAdBlockResourceComponentPublicKeySHA256[32] = {
    0xc5, 0x33, 0x81, 0xc1, 0xbc, 0x12, 0x2f, 0x03, 0x5d, 0x36, 0x0a,
    0x8e, 0xfc, 0xc7, 0x41, 0xef, 0xc6, 0x97, 0x1a, 0x07, 0x29, 0x1c,
    0x97, 0x2c, 0x18, 0x2e, 0x1a, 0xd5, 0x76, 0xa6, 0x59, 0xfc};
static_assert(std::size(kAdBlockResourceComponentPublicKeySHA256) ==
              crypto::kSHA256Length);

inline constexpr char kAdBlockFilterListCatalogComponentName[] =
    "Brave Ad Block List Catalog";
inline constexpr char kAdBlockFilterListCatalogComponentId[] =
    "gkboaolpopklhgplhaaiboijnklogmbc";
inline constexpr uint8_t kAdBlockFilterListCatalogComponentPublicKeySHA256[32] =
    {0x6a, 0x1e, 0x0e, 0xbf, 0xef, 0xab, 0x76, 0xfb, 0x70, 0x08, 0x1e,
     0x89, 0xda, 0xbe, 0x6c, 0x12, 0xd2, 0x32, 0x31, 0x85, 0xec, 0x24,
     0xb0, 0x3d, 0x11, 0xe4, 0x69, 0x45, 0xb5, 0x29, 0x95, 0x62};
static_assert(std::size(kAdBlockFilterListCatalogComponentPublicKeySHA256) ==
              crypto::kSHA256Length);

inline constexpr char kCookieListEnabledHistogram[] =
    "Brave.Shields.CookieListEnabled";
inline constexpr char kCookieListPromptHistogram[] =
    "Brave.Shields.CookieListPrompt";

// The list of UUIDs of filter lists that will be loaded by AdBlockOnlyMode.
inline constexpr auto kAdblockOnlyModeFilterListUUIDs =
    base::MakeFixedFlatSet<std::string_view>(
        {kDefaultAdblockFiltersListUuid, kFirstPartyAdblockFiltersListUuid,
         kAdblockOnlySupplementalListUuid});

// The list of language codes that are supported by Ad Block Only mode.
inline constexpr auto kAdblockOnlyModeSupportedLanguageCodes =
    base::MakeFixedFlatSet<std::string_view>({"en"});

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_CONSTANTS_H_
