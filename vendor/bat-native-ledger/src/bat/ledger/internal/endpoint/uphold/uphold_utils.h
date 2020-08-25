/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_UPHOLD_UPHOLD_UTILS_H_
#define BRAVELEDGER_ENDPOINT_UPHOLD_UPHOLD_UTILS_H_

#include <string>
#include <vector>

namespace ledger {
namespace endpoint {
namespace uphold {

std::string GetClientSecret();

std::vector<std::string> RequestAuthorization(const std::string& token = "");

std::string GetServerUrl(const std::string& path);

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_UPHOLD_UPHOLD_UTILS_H_
