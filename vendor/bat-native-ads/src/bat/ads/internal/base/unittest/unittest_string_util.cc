/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/unittest/unittest_string_util.h"

#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace ads {

std::string CapitalizeFirstCharacterOfEachWordAndTrimWhitespace(
    const std::string& value) {
  std::vector<std::string> components = base::SplitString(
      value, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (auto& component : components) {
    component[0] = base::ToUpperASCII(component[0]);
  }

  return base::StrCat(components);
}

}  // namespace ads
