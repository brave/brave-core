/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/bookmark_order_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace brave_sync {

std::vector<int> OrderToIntVect(const std::string &s) {
  std::vector<std::string> vec_s = SplitString(
  s,
  ".",
  base::WhitespaceHandling::TRIM_WHITESPACE,
  base::SplitResult::SPLIT_WANT_NONEMPTY);
  std::vector<int> vec_int;
  vec_int.reserve(vec_s.size());
  for (size_t i = 0; i < vec_s.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(vec_s[i], &output);
    CHECK(b);
    CHECK(output >= 0);
    vec_int.emplace_back(output);
  }
  return vec_int;
}

bool CompareOrder(const std::string &left, const std::string &right) {
  // Return: true if left <  right
  // Split each and use C++ stdlib
  std::vector<int> vec_left = OrderToIntVect(left);
  std::vector<int> vec_right = OrderToIntVect(right);

  return std::lexicographical_compare(vec_left.begin(), vec_left.end(),
    vec_right.begin(), vec_right.end());
}

bool IsOrdered(const std::vector<std::string> &vec) {
  bool ret = std::is_sorted(std::begin(vec), std::end(vec), CompareOrder);
  return ret;
}

std::string ToOrderString(const std::vector<int> &vec_int) {
  std::string ret;
  for (size_t i = 0; i < vec_int.size(); ++i) {
    if (vec_int[i] < 0) {
      return "";
    }
    ret += std::to_string(vec_int[i]);
    if (i != vec_int.size() - 1) {
      ret += ".";
    }
  }
  return ret;
}

int GetPositionToInsert(const std::vector<std::string> &vec, const std::string &order) {
  DCHECK(IsOrdered(vec));
  auto it = std::lower_bound(std::begin(vec), std::end(vec), order, CompareOrder);
  int pos = std::distance(std::begin(vec), it);
  return pos;
}

} // namespace brave_sync
