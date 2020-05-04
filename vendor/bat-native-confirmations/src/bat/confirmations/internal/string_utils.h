/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_STRING_UTILS_H_
#define BAT_CONFIRMATIONS_INTERNAL_STRING_UTILS_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace confirmations {

std::vector<uint8_t> DecodeHexString(
    const std::string& string);

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_STRING_UTILS_H_
