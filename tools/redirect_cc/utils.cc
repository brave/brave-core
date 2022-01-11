/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/tools/redirect_cc/utils.h"

#include <iostream>

#include "brave/tools/redirect_cc/os_utils.h"

namespace utils {

bool StartsWith(FilePathStringView str, FilePathStringView search_for) {
  return str.size() >= search_for.size() &&
         str.compare(0, search_for.size(), search_for) == 0;
}

bool EndsWith(FilePathStringView str, FilePathStringView search_for) {
  return str.size() >= search_for.size() &&
         str.compare(str.size() - search_for.size(), FilePathStringView::npos,
                     search_for) == 0;
}

}  // namespace utils
