/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_CONSTANTS_H_

namespace brave_account {

inline constexpr char kInitiatingServiceNameQueryParam[] =
    "initiating-service-name";

// Path the Brave Account WebUI serves the Settings rows at. Only used on
// Android/iOS, where Settings is native.
inline constexpr char kSettingsPath[] = "settings";

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_CONSTANTS_H_
