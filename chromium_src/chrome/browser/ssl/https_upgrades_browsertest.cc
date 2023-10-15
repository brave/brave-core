/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/base/features.h"

// Prevent re-defining InitWithFeatures definition.
#include "base/test/scoped_feature_list.h"

#define InitWithFeatures(...) \
  InitWithFeaturesAndDisable(net::features::kBraveHttpsByDefault, __VA_ARGS__)

#include "src/chrome/browser/ssl/https_upgrades_browsertest.cc"
#undef InitWithFeatures
