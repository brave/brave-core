/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/common/pref_names.h"

namespace brave_shields {
namespace prefs {

const char kAdBlockCheckedDefaultRegion[] =
    "brave.ad_block.checked_default_region";
const char kAdBlockCheckedAllDefaultRegions[] =
    "brave.ad_block.checked_all_default_regions";
const char kAdBlockCookieListOptInShown[] =
    "brave.ad_block.cookie_list_opt_in_shown";
const char kAdBlockCookieListSettingTouched[] =
    "brave.ad_block.cookie_list_setting_touched";
const char kAdBlockMobileNotificationsListSettingTouched[] =
    "brave.ad_block.mobile_notifications_list_setting_touched";

const char kAdBlockCustomFilters[] = "brave.ad_block.custom_filters";
const char kAdBlockRegionalFilters[] = "brave.ad_block.regional_filters";
const char kAdBlockListSubscriptions[] = "brave.ad_block.list_subscriptions";
const char kFBEmbedControlType[] = "brave.fb_embed_default";
const char kTwitterEmbedControlType[] = "brave.twitter_embed_default";
const char kLinkedInEmbedControlType[] = "brave.linkedin_embed_default";
const char kReduceLanguageEnabled[] = "brave.reduce_language";

}  // namespace prefs
}  // namespace brave_shields
