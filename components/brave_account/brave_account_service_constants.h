/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_CONSTANTS_H_

#include "base/time/time.h"

namespace brave_account {

inline constexpr base::TimeDelta kVerifyResultPollInterval = base::Seconds(5);
inline constexpr base::TimeDelta kVerifyResultWatchdogInterval =
    3 * kVerifyResultPollInterval;

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_CONSTANTS_H_
