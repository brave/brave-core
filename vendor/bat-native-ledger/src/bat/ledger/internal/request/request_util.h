/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_COMMON_REQUEST_UTIL_H_
#define BRAVELEDGER_COMMON_REQUEST_UTIL_H_

#include <string>

namespace braveledger_request_util {

enum class ServerTypes {
  LEDGER,
  BALANCE,
  PUBLISHER,
  PUBLISHER_DISTRO
};

std::string BuildUrl(
    const std::string& path,
    const std::string& prefix = "",
    const ServerTypes& server = ServerTypes::LEDGER);

}  // namespace braveledger_request_util

#endif  // BRAVELEDGER_COMMON_REQUEST_UTIL_H_
