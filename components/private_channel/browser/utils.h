/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_UTILS_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_UTILS_H_

#include <string>

namespace private_channel {

std::string convert_to_str_array(const uint8_t* ptr, int size);

}  // namespace private_channel

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_UTILS_H_
