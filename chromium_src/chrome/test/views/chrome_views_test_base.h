/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_VIEWS_CHROME_VIEWS_TEST_BASE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_VIEWS_CHROME_VIEWS_TEST_BASE_H_

#include <memory>

#include "base/auto_reset.h"
#include "ui/gfx/animation/animation.h"

#define ChromeViewsTestBase ChromeViewsTestBase_ChromiumImpl

#include "src/chrome/test/views/chrome_views_test_base.h"  // IWYU pragma: export

#undef ChromeViewsTestBase

class ChromeViewsTestBase : public ChromeViewsTestBase_ChromiumImpl {
 public:
  ChromeViewsTestBase();
  ChromeViewsTestBase(const ChromeViewsTestBase&) = delete;
  ChromeViewsTestBase& operator=(const ChromeViewsTestBase&) = delete;
  ~ChromeViewsTestBase() override;

  // views::ViewsTestBase:
  void SetUp() override;
  void TearDown() override;

 private:
  std::unique_ptr<base::AutoReset<gfx::Animation::RichAnimationRenderMode>>
      animation_mode_reset_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_VIEWS_CHROME_VIEWS_TEST_BASE_H_
