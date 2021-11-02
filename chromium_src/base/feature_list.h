/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_FEATURE_LIST_H_
#define BRAVE_CHROMIUM_SRC_BASE_FEATURE_LIST_H_

// Make |default_state| mutable via a union, so it can be modified, but only
// explicitly through |mutable_default_state| counterpart.
// See also: brave/chromium_src/base/feature_override.h
#define default_state                                             \
  *NotUsed() const { return nullptr; }                            \
                                                                  \
  constexpr Feature(const char* name, FeatureState default_state) \
      : name(name), mutable_default_state(default_state) {}       \
                                                                  \
  union {                                                         \
    mutable FeatureState mutable_default_state;                   \
    const FeatureState default_state;                             \
  }

#include "../../../base/feature_list.h"

#undef default_state

#endif  // BRAVE_CHROMIUM_SRC_BASE_FEATURE_LIST_H_
