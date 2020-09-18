/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <iostream>
#include <string>

#include "base/strings/string_tokenizer.h"
#include "brave/components/private_channel/utils.h"

std::string convert_to_str(const uint8_t* ptr, int size) {
  std::string str;
  for (int i = 0; i < size; i++) {
    str.append(std::to_string(static_cast<int>(ptr[i])));
    if (i != size - 1) {
      str.append(", ");
    }
  }
  str.insert(0, "[");
  str.append("]");
  return str;
}
