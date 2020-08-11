/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LEGACY_BAT_UTIL_H_
#define BRAVELEDGER_LEGACY_BAT_UTIL_H_

#include <string>

namespace braveledger_bat_util {

std::string ConvertToProbi(const std::string& amount);

double ProbiToDouble(const std::string& probi);

}  // namespace braveledger_bat_util

#endif  // BRAVELEDGER_LEGACY_BAT_UTIL_H_
