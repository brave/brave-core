/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_UTILS_H_
#define BRAVE_COMPONENTS_TOR_TOR_UTILS_H_

class PrefService;

namespace tor {

void MigrateLastUsedProfileFromLocalStatePrefs(PrefService* local_state);

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_UTILS_H_
