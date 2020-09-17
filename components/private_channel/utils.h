/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_UTILS_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_UTILS_H_

#include <string>

std::string convert_to_str(const uint8_t* ptr, int size);

void parse_str_response(const char* ptr, uint8_t* dst);

int get_size_response(const char* ptr);

const uint8_t* parse_str_response_u8pointer(std::string str);

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_UTILS_H_
