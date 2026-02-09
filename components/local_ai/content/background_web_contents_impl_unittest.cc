// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/content/background_web_contents_impl.h"

#include <memory>

#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace local_ai {

class MockDelegate : public BackgroundWebContents::Delegate {
 public:
  MOCK_METHOD(void, OnBackgroundContentsReady, (), (override));
  MOCK_METHOD(void,
              OnBackgroundContentsDestroyed,
              (BackgroundWebContents::DestroyReason reason),
              (override));
};

class BackgroundWebContentsImplTest
    : public content::RenderViewHostTestHarness {
 protected:
  void TearDown() override {
    background_contents_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  void CreateContents(const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    background_contents_ = std::make_unique<BackgroundWebContentsImpl>(
        browser_context(), GURL("chrome-untrusted://test/"), &delegate_);
  }

  // Navigate the background WebContents to trigger DidFinishLoad.
  void SimulatePageLoad(const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    auto* wc = background_contents_->web_contents();
    ASSERT_NE(nullptr, wc);
    content::NavigationSimulator::NavigateAndCommitFromBrowser(
        wc, GURL("chrome-untrusted://test/"));
  }

  testing::NiceMock<MockDelegate> delegate_;
  std::unique_ptr<BackgroundWebContentsImpl> background_contents_;
};

TEST_F(BackgroundWebContentsImplTest, ConstructionCreatesWebContents) {
  CreateContents();
  EXPECT_NE(nullptr, background_contents_->web_contents());
}

TEST_F(BackgroundWebContentsImplTest, DidFinishLoadCallsReady) {
  CreateContents();

  EXPECT_CALL(delegate_, OnBackgroundContentsReady());

  SimulatePageLoad();
}

TEST_F(BackgroundWebContentsImplTest,
       CloseContentsCallsOnBackgroundContentsDestroyed) {
  CreateContents();

  EXPECT_CALL(delegate_, OnBackgroundContentsDestroyed(
                             BackgroundWebContents::DestroyReason::kClose));

  // CloseContents is called when window.close() fires.
  static_cast<content::WebContentsDelegate*>(background_contents_.get())
      ->CloseContents(background_contents_->web_contents());
}

TEST_F(BackgroundWebContentsImplTest, UnexpectedUrlCallsDestroyed) {
  CreateContents();

  EXPECT_CALL(delegate_, OnBackgroundContentsReady()).Times(0);
  EXPECT_CALL(delegate_,
              OnBackgroundContentsDestroyed(
                  BackgroundWebContents::DestroyReason::kInvalidUrl));

  // Navigate to a different URL than expected.
  auto* wc = background_contents_->web_contents();
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      wc, GURL("chrome-untrusted://wrong/"));
}

TEST_F(BackgroundWebContentsImplTest, DestructorDoesNotFireDelegateCallbacks) {
  CreateContents();
  // Destroying should not crash or fire delegate callbacks.
  background_contents_.reset();
}

}  // namespace local_ai
