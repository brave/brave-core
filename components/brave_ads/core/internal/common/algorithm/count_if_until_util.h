/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_ALGORITHM_COUNT_IF_UNTIL_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_ALGORITHM_COUNT_IF_UNTIL_UTIL_H_

#include <cstddef>
#include <iterator>

namespace brave_ads {

template <typename T>
concept Iterable = requires(T t) {
  std::cbegin(t);
  std::cend(t);
};

// Iterates over the elements of a generic iterable container. For each element,
// it checks if the predicate function returns true. If it does, it increments
// a counter. The loop breaks when the counter reaches the stop count.
//
// Example usage:
//
//   std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//
//   auto is_even = [](int number) { return number % 2 == 0; };
//   size_t stop_count = 5;
//
//   size_t count = count_if_until(numbers, is_even, stop_count);

template <Iterable Container, typename Predicate>
size_t count_if_until(const Container& container,  // NOLINT
                      const Predicate& predicate,
                      size_t stop_count) {
  size_t count = 0;

  for (const auto& element : container) {
    if (count == stop_count) {
      break;
    }

    if (predicate(element)) {
      ++count;
    }
  }

  return count;
}

// Iterates over the elements of a generic iterable container. For each element,
// it checks if the stop predicate function returns true. If it does, it breaks
// the loop. Otherwise, it checks if the predicate function returns true. If it
// does, it increments a counter. This function is useful when the stop
// condition is not a simple count, but a more complex condition.
//
// Example usage:
//
//   std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//
//   auto is_even = [](int number) { return number % 2 == 0; };
//   auto stop_at_five = [](size_t count) { return count == 5; };
//
//   size_t count = count_if_until(numbers, is_even, stop_at_five);

template <Iterable Container, typename Predicate, typename StopPredicate>
size_t count_if_until(const Container& container,  // NOLINT
                      const Predicate& predicate,
                      const StopPredicate& stop_predicate) {
  size_t count = 0;

  for (const auto& element : container) {
    if (stop_predicate(count)) {
      break;
    }

    if (predicate(element)) {
      ++count;
    }
  }

  return count;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_ALGORITHM_COUNT_IF_UNTIL_UTIL_H_
