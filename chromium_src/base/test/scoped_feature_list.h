/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_TEST_SCOPED_FEATURE_LIST_H_
#define BRAVE_CHROMIUM_SRC_BASE_TEST_SCOPED_FEATURE_LIST_H_

#define InitWithFeatures(...)                                      \
  InitWithFeaturesAndDisable(const FeatureRef& feature_to_disable, \
                             __VA_ARGS__);                         \
  void InitWithFeatures(__VA_ARGS__)

#include "src/base/test/scoped_feature_list.h"  // IWYU pragma: export

#undef InitWithFeatures

#endif  // BRAVE_CHROMIUM_SRC_BASE_TEST_SCOPED_FEATURE_LIST_H_
