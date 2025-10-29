/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GLOBAL_PRIVACY_CONTROL_GLOBAL_PRIVACY_CONTROL_UTILS_H_
#define BRAVE_COMPONENTS_GLOBAL_PRIVACY_CONTROL_GLOBAL_PRIVACY_CONTROL_UTILS_H_

class PrefService;

namespace global_privacy_control {

bool IsGlobalPrivacyControlEnabled(PrefService* prefs);

}  // namespace global_privacy_control

#endif  // BRAVE_COMPONENTS_GLOBAL_PRIVACY_CONTROL_GLOBAL_PRIVACY_CONTROL_UTILS_H_
