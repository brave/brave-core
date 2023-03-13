/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_CONTAINER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_CONTAINER_UTIL_H_

#include <cstddef>

namespace brave_ads {

// Checks that |lhs| and |rhs| contain the same number of elements and each
// element in |lhs| is present in |rhs| and vice-versa (Uses the == operator for
// comparing). Returns true if it is the case. Note that this method will return
// true for (aab, abb).
template <typename T>
bool ContainersEq(const T& lhs, const T& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t i = 0; i < rhs.size(); i++) {
    bool found = false;

    for (size_t j = 0; (j < rhs.size()) && !found; j++) {
      found = found || lhs[i] == rhs[j];
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_CONTAINER_UTIL_H_
