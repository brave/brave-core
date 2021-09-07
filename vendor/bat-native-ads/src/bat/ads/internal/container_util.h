/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONTAINER_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONTAINER_UTIL_H_

#include <deque>
#include <vector>

#include "base/check_op.h"

namespace ads {

template <typename T>
std::deque<T> VectorToDeque(const std::vector<T>& vector) {
  std::deque<T> deque;

  for (const auto& element : vector) {
    deque.push_back(element);
  }

  return deque;
}

template <typename T>
std::vector<std::vector<T>> SplitVector(const std::vector<T>& elements,
                                        const int chunk_size) {
  DCHECK_NE(0, chunk_size);

  std::vector<std::vector<T>> result;
  result.reserve((elements.size() + chunk_size - 1) / chunk_size);

  auto begin = elements.begin();
  const auto end = elements.end();
  while (begin != end) {
    const auto next =
        std::distance(begin, end) >= chunk_size ? begin + chunk_size : end;

    result.emplace_back(begin, next);

    begin = next;
  }

  return result;
}

template <typename T>
bool CompareMaps(const T& lhs, const T& rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// Checks that |c1| and |c2| contain the same number of elements and each
// element in |c1| is present in |c2| and vice-versa (Uses the == operator for
// comparing). Returns true if it is the case. Note that this method will return
// true for (aab, abb)
template <typename T>
bool CompareAsSets(const T& c1, const T& c2) {
  if (c1.size() != c2.size()) {
    return false;
  }

  for (size_t i = 0; i < c2.size(); i++) {
    bool found = false;

    for (size_t j = 0; (j < c2.size()) && !found; j++) {
      found = found || (c1[i] == c2[j]);
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONTAINER_UTIL_H_
