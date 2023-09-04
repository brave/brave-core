/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/base/features.h"

#include "base/test/scoped_feature_list.h"

#define InitWithFeaturesAndParameters(A, B) \
  InitWithFeaturesAndParameters(A, {net::features::kBraveHttpsByDefault})

#include "src/chrome/browser/ui/views/side_panel/search_companion/companion_page_browsertest.cc"

#undef InitWithFeaturesAndParameters
