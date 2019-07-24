/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/bookmark_order_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace brave_sync {

namespace {

bool CompareOrder(const std::vector<int>& vec_left,
    const std::vector<int>& vec_right) {
  // Use C++ stdlib
  return std::lexicographical_compare(vec_left.begin(), vec_left.end(),
    vec_right.begin(), vec_right.end());
}

}  // namespace

std::vector<int> OrderToIntVect(const std::string& s) {
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
    CHECK_GE(output, 0);
    vec_int.emplace_back(output);
  }
  return vec_int;
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

bool CompareOrder(const std::string& left, const std::string& right) {
  // Return: true if left <  right
  // Split each and compare as int vectors
  std::vector<int> vec_left = OrderToIntVect(left);
  std::vector<int> vec_right = OrderToIntVect(right);

  return CompareOrder(vec_left, vec_right);
}

namespace {

std::string GetNextOrderFromPrevOrder(std::vector<int>* vec_prev) {
  DCHECK_GT(vec_prev->size(), 2u);
  int last_number = vec_prev->at(vec_prev->size() - 1);
  DCHECK_GT(last_number, 0);
  if (last_number <= 0) {
    return "";
  } else {
    vec_prev->at(vec_prev->size() - 1)++;
    return ToOrderString(*vec_prev);
  }
}

std::string GetPrevOrderFromNextOrder(std::vector<int>* vec_next) {
  DCHECK_GT(vec_next->size(), 2u);
  int last_number = vec_next->at(vec_next->size() - 1);
  DCHECK_GT(last_number, 0);
  vec_next->resize(vec_next->size() - 1);
  if (last_number <= 0) {
    return "";
  } else if (last_number == 1) {
    return ToOrderString(*vec_next) + ".0.1";
  } else {
    vec_next->push_back(last_number - 1);
    return ToOrderString(*vec_next);
  }
}

}  // namespace

// Inspired by https://github.com/brave/sync/blob/staging/client/bookmarkUtil.js
std::string GetOrder(const std::string& prev, const std::string& next,
    const std::string& parent) {
  if (prev.empty() && next.empty()) {
    DCHECK(!parent.empty());
    return parent + ".1";
  } else if (!prev.empty() && next.empty()) {
    std::vector<int> vec_prev = OrderToIntVect(prev);
    DCHECK_GT(vec_prev.size(), 2u);
    // Just increase the last number, as we don't have next
    return GetNextOrderFromPrevOrder(&vec_prev);
  } else if (prev.empty() && !next.empty()) {
    std::vector<int> vec_next = OrderToIntVect(next);
    DCHECK_GT(vec_next.size(), 2u);
    // Just decrease the last number or substitute with 0.1,
    // as we don't have prev
    return GetPrevOrderFromNextOrder(&vec_next);
  } else {
    DCHECK(!prev.empty() && !next.empty());
    std::vector<int> vec_prev = OrderToIntVect(prev);
    DCHECK_GT(vec_prev.size(), 2u);
    std::vector<int> vec_next = OrderToIntVect(next);
    DCHECK_GT(vec_next.size(), 2u);
    DCHECK(CompareOrder(prev, next));

    // Assume prev looks as a.b.c.d
    // result candidates are:
    // a.b.c.(d+1)
    // a.b.c.d.1
    // a.b.c.d.0.1
    // a.b.c.d.0.0.1
    // ...
    // each of them is greater than prev

    // Length of result in worse case can be one segment longer
    // than length of next
    // And result should be < next

    std::vector<int> vec_result;
    vec_result = vec_prev;
    vec_result[vec_result.size() - 1]++;

    // Case a.b.c.(d+1)
    DCHECK(CompareOrder(vec_prev, vec_result));
    if (CompareOrder(vec_result, vec_next)) {
      return ToOrderString(vec_result);
    }

    vec_result = vec_prev;
    vec_result.push_back(1);
    // Case a.b.c.d.1
    DCHECK(CompareOrder(vec_prev, vec_result));
    if (CompareOrder(vec_result, vec_next)) {
      return ToOrderString(vec_result);
    }

    size_t insert_at = vec_prev.size();
    size_t try_until_size = vec_next.size() + 1;
    // Cases a.b.c.d.0....0.1
    while (vec_result.size() < try_until_size) {
      vec_result.insert(vec_result.begin() + insert_at, 0);
      DCHECK(CompareOrder(vec_prev, vec_result));
      if (CompareOrder(vec_result, vec_next)) {
        return ToOrderString(vec_result);
      }
    }

    NOTREACHED() << "[BraveSync] " << __func__ << " prev=" << prev <<
        " next=" << next << " terminated with " << ToOrderString(vec_result);
  }

  NOTREACHED() << "[BraveSync] " << __func__ <<
      " condition is not handled prev.empty()=" << prev.empty() <<
      " next.empty()=" << next.empty();

  return "";
}

}  // namespace brave_sync
