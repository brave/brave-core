/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification_helper.h"

#include "base/strings/string_split.h"

namespace helper {

std::vector<std::string> Classification::GetClassifications(
    const std::string& classification) {
  return base::SplitString(classification, "-", base::KEEP_WHITESPACE,
                           base::SPLIT_WANT_ALL);
}

}  // namespace helper
