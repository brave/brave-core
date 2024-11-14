// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_PREF_NAMES_H_

namespace brave_shields {
namespace prefs {

inline constexpr char kAdBlockCheckedDefaultRegion[] =
    "brave.ad_block.checked_default_region";
inline constexpr char kAdBlockCheckedAllDefaultRegions[] =
    "brave.ad_block.checked_all_default_regions";
inline constexpr char kAdBlockCookieListOptInShown[] =
    "brave.ad_block.cookie_list_opt_in_shown";
inline constexpr char kAdBlockCookieListSettingTouched[] =
    "brave.ad_block.cookie_list_setting_touched";
inline constexpr char kAdBlockMobileNotificationsListSettingTouched[] =
    "brave.ad_block.mobile_notifications_list_setting_touched";

inline constexpr char kAdBlockCustomFilters[] = "brave.ad_block.custom_filters";
inline constexpr char kAdBlockRegionalFilters[] =
    "brave.ad_block.regional_filters";
inline constexpr char kAdBlockListSubscriptions[] =
    "brave.ad_block.list_subscriptions";
inline constexpr char kAdBlockDeveloperMode[] = "brave.ad_block.developer_mode";

inline constexpr char kFBEmbedControlType[] = "brave.fb_embed_default";
inline constexpr char kTwitterEmbedControlType[] =
    "brave.twitter_embed_default";
inline constexpr char kLinkedInEmbedControlType[] =
    "brave.linkedin_embed_default";
inline constexpr char kReduceLanguageEnabled[] = "brave.reduce_language";

}  // namespace prefs
}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_PREF_NAMES_H_
