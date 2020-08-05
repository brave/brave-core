/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/classification_util.h"

#include "base/strings/string_split.h"

namespace ads {
namespace classification {

std::vector<std::string> SplitCategory(
    const std::string& category) {
  return base::SplitString(category, kCategorySeparator, base::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
}

}  // namespace classification
}  // namespace ads
