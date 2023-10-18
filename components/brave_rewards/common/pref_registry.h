/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_REGISTRY_H_

class PrefRegistrySimple;

namespace brave_rewards {

void RegisterProfilePrefs(PrefRegistrySimple* registry);
void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_REGISTRY_H_
