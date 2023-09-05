/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/test/views/chrome_views_test_base.h"

#include "ui/gfx/animation/animation_test_api.h"

#define ChromeViewsTestBase ChromeViewsTestBase_ChromiumImpl

#include "src/chrome/test/views/chrome_views_test_base.cc"

#undef ChromeViewsTestBase

ChromeViewsTestBase::ChromeViewsTestBase() = default;

ChromeViewsTestBase::~ChromeViewsTestBase() = default;

void ChromeViewsTestBase::SetUp() {
  if (const testing::TestInfo* test_info =
          testing::UnitTest::GetInstance()->current_test_info()) {
    // Force animations to be always enabled to not fail some upstream tests
    // when run under RDP session.
    static constexpr base::StringPiece kRichAnimationForcedSuites[] = {
        "CompoundTabContainerTest",
        "TabContainerTest",
    };
    if (base::Contains(kRichAnimationForcedSuites,
                       test_info->test_suite_name())) {
      animation_mode_reset_ = gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_ENABLED);
    }
  }
  ChromeViewsTestBase_ChromiumImpl::SetUp();
}

void ChromeViewsTestBase::TearDown() {
  ChromeViewsTestBase_ChromiumImpl::TearDown();
  animation_mode_reset_.reset();
}
