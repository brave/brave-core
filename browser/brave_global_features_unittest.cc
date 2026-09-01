/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_global_features.h"

#include "chrome/browser/global_features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Mirrors the fakes upstream tests install for GlobalFeatures.
class ForeignGlobalFeatures : public GlobalFeatures {};

}  // namespace

TEST(BraveGlobalFeaturesTest, FromGlobalFeaturesRejectsForeignSubclass) {
  ForeignGlobalFeatures foreign;
  EXPECT_EQ(nullptr, BraveGlobalFeatures::FromGlobalFeatures(&foreign));
  EXPECT_EQ(nullptr, BraveGlobalFeatures::FromGlobalFeatures(nullptr));

  BraveGlobalFeatures brave;
  EXPECT_EQ(&brave, BraveGlobalFeatures::FromGlobalFeatures(&brave));
}
