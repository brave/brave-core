/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_location_bar_model_delegate.h"

#include "components/omnibox/browser/vector_icons.h"
#include "testing/gtest/include/gtest/gtest.h"

class TestLocationBarModelDelegate
    : public BraveBrowserLocationBarModelDelegate {
 public:
  TestLocationBarModelDelegate()
      : BraveBrowserLocationBarModelDelegate(nullptr) {}
  ~TestLocationBarModelDelegate() override {}

  bool GetURL(GURL* url) const override {
    *url = GURL("brave://sync/");
    return true;
  }
};

// Check proper icon is used for brave url.
TEST(BraveBrowserLocationBarModelDelegateTest, VectorIconOverrideTest) {
  TestLocationBarModelDelegate delegate;
  EXPECT_EQ(&omnibox::kProductIcon,
            static_cast<LocationBarModelDelegate*>(&delegate)->
                GetVectorIconOverride());
}
