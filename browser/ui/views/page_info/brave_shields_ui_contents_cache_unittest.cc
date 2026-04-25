/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_shields_ui_contents_cache.h"

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/test/task_environment.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Minimal test implementation of WebUIContentsWrapper.
class TestWebUIContentsWrapper : public WebUIContentsWrapper {
 public:
  explicit TestWebUIContentsWrapper(Profile* profile)
      : WebUIContentsWrapper(GURL(""),
                             profile,
                             /*task_manager_string_id=*/0,
                             /*webui_resizes_host=*/true,
                             /*esc_closes_ui=*/true,
                             /*supports_draggable_regions=*/false,
                             "TestWebUI") {}
  ~TestWebUIContentsWrapper() override = default;

  // WebUIContentsWrapper:
  void ReloadWebContents() override {}
  base::WeakPtr<WebUIContentsWrapper> GetWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<TestWebUIContentsWrapper> weak_ptr_factory_{this};
};

}  // namespace

class BraveShieldsUIContentsCacheTest : public ChromeViewsTestBase {
 public:
  BraveShieldsUIContentsCacheTest() = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    ChromeViewsTestBase::SetUp();
  }

 protected:
  std::unique_ptr<WebUIContentsWrapper> CreateTestWrapper() {
    return std::make_unique<TestWebUIContentsWrapper>(profile_.get());
  }

  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveShieldsUIContentsCacheTest, CachedContentsCanBeRetrieved) {
  BraveShieldsUIContentsCache cache;

  auto wrapper = CreateTestWrapper();
  WebUIContentsWrapper* raw_ptr = wrapper.get();

  cache.CacheShieldsUIContents(std::move(wrapper));
  auto retrieved = cache.GetCachedShieldsUIContents();

  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved.get(), raw_ptr);
}

TEST_F(BraveShieldsUIContentsCacheTest, CacheExpiresAfterTimeout) {
  BraveShieldsUIContentsCache cache;

  cache.CacheShieldsUIContents(CreateTestWrapper());

  // Advance time past the 30-second expiry interval.
  task_environment()->FastForwardBy(base::Seconds(31));

  auto retrieved = cache.GetCachedShieldsUIContents();
  EXPECT_EQ(retrieved, nullptr);
}

TEST_F(BraveShieldsUIContentsCacheTest, ResetClearsCache) {
  BraveShieldsUIContentsCache cache;

  cache.CacheShieldsUIContents(CreateTestWrapper());
  cache.ResetCachedShieldsUIContents();

  auto retrieved = cache.GetCachedShieldsUIContents();
  EXPECT_EQ(retrieved, nullptr);
}
