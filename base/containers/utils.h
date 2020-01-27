/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_CONTAINERS_UTILS_H_
#define BRAVE_BASE_CONTAINERS_UTILS_H_

// The following code used to live in /mojo/public/cpp/bindings/map.h but has
// been removed in https://chromium.googlesource.com/chromium/src/+/8d90918
// because Chromium switched to using base::flat_map instead of std::map in the
// code that used with mojo. Namespace in the code below has been changed from
// `mojo` to `base`.

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <utility>

#include "base/containers/flat_map.h"

namespace base {

template <typename Key, typename Value>
base::flat_map<Key, Value> MapToFlatMap(const std::map<Key, Value>& input) {
  return base::flat_map<Key, Value>(input.begin(), input.end());
}

template <typename Key, typename Value>
base::flat_map<Key, Value> MapToFlatMap(std::map<Key, Value>&& input) {
  return base::flat_map<Key, Value>(std::make_move_iterator(input.begin()),
                                    std::make_move_iterator(input.end()));
}

template <typename Key, typename Value>
std::map<Key, Value> FlatMapToMap(const base::flat_map<Key, Value>& input) {
  return std::map<Key, Value>(input.begin(), input.end());
}

template <typename Key, typename Value>
std::map<Key, Value> FlatMapToMap(base::flat_map<Key, Value>&& input) {
  return std::map<Key, Value>(std::make_move_iterator(input.begin()),
                              std::make_move_iterator(input.end()));
}

}  // namespace base

#endif  // BRAVE_BASE_CONTAINERS_UTILS_H_
