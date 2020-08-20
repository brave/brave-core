/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_REWARDS_REWARDS_UTIL_H_
#define BRAVELEDGER_ENDPOINT_REWARDS_REWARDS_UTIL_H_

#include <string>

namespace ledger {
namespace endpoint {
namespace rewards {

std::string GetServerUrl(const std::string& path);

}  // namespace rewards
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_REWARDS_REWARDS_UTIL_H_
