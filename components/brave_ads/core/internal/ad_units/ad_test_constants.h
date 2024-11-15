/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_AD_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_AD_TEST_CONSTANTS_H_

namespace brave_ads::test {

inline constexpr char kPlacementId[] = "9bac9ae4-693c-4569-9b3e-300e357780cf";
inline constexpr char kAnotherPlacementId[] =
    "75994a99-65e4-4171-b134-10859dfc6d18";
inline constexpr char kMissingPlacementId[] =
    "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
inline constexpr char kInvalidPlacementId[] = "";

inline constexpr char kCreativeInstanceId[] =
    "546fe7b0-5047-4f28-a11c-81f14edcf0f6";
inline constexpr char kAnotherCreativeInstanceId[] =
    "e1c4c9a3-a05b-4d6a-83f9-72260729ebb0";
inline constexpr char kMissingCreativeInstanceId[] =
    "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
inline constexpr char kInvalidCreativeInstanceId[] = "";

inline constexpr char kCreativeSetId[] = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
inline constexpr char kAnotherCreativeSetId[] =
    "e31e1905-e8a3-4461-9930-82f68e70d8ba";
inline constexpr char kMissingCreativeSetId[] =
    "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
inline constexpr char kInvalidCreativeSetId[] = "";

inline constexpr char kCampaignId[] = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
inline constexpr char kAnotherCampaignId[] =
    "b164214f-670d-4eb2-8a71-92fb2dd805ca";
inline constexpr char kMissingCampaignId[] =
    "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
inline constexpr char kInvalidCampaignId[] = "";

inline constexpr char kAdvertiserId[] = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
inline constexpr char kAnotherAdvertiserId[] =
    "5b010b85-39ed-476a-b8c1-1746db2f86b7";
inline constexpr char kMissingAdvertiserId[] =
    "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
inline constexpr char kInvalidAdvertiserId[] = "";

inline constexpr char kSegment[] = "untargeted";

inline constexpr char kTargetUrl[] = "https://brave.com";

inline constexpr char kTitle[] = "Test Ad Title";
inline constexpr char kDescription[] = "Test Ad Description";

inline constexpr double kValue = 1.0;

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_AD_TEST_CONSTANTS_H_
