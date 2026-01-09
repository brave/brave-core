/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_CONSTANTS_H_

#include "base/time/time.h"

namespace brave_account {

inline constexpr base::TimeDelta kVerifyResultPollInterval = base::Seconds(5);
inline constexpr base::TimeDelta kAuthValidatePollInterval = base::Minutes(2);

// If a polling request (VerifyResult or AuthValidate) doesn't complete within
// this interval, it will be canceled and retried to prevent hung requests from
// stopping the periodic polling indefinitely.
inline constexpr base::TimeDelta kWatchdogInterval = base::Seconds(15);

// Service tokens are valid for 6 days.
// Use a threshold that ensures we always return a service token
// with at least 10 minutes remaining.
inline constexpr base::TimeDelta kServiceTokenMaxAge =
    base::Days(6) - base::Minutes(10);

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_CONSTANTS_H_
