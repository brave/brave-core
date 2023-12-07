/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PLAYER_CORE_COMMON_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_PLAYER_CORE_COMMON_PREFS_H_

#include "base/component_export.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_player {

COMPONENT_EXPORT(BRAVE_PLAYER_COMMON)
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

// A dictionary pref that contains host of sites and timestamp. The timestamp
// is base::Time serialized with `base::TimeToValue(Time time);` in
// `//base/json/values_util.h`
constexpr char kBravePlayerAdBlockAdjustmentDisplayedSites[] =
    "brave_player.adblock_adjustment_displayed_sites";

}  // namespace brave_player

#endif  // BRAVE_COMPONENTS_BRAVE_PLAYER_CORE_COMMON_PREFS_H_
