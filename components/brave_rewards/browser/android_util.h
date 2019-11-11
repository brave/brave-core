/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_ANDROID_ANDROID_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_ANDROID_ANDROID_UTIL_H_

#include <string>

#include "bat/ledger/ledger_client.h"

namespace android_util {

ledger::ClientInfoPtr GetAndroidClientInfo();

std::string ParseClaimPromotionResponse(const std::string& response);

}  // namespace android_util

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_ANDROID_ANDROID_UTIL_H_
