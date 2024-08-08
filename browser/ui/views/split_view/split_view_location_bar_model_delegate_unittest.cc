/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_location_bar_model_delegate.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

class SplitViewLocationBarModelDelegateUnitTest : public testing::Test {
 public:
  SplitViewLocationBarModelDelegateUnitTest() = default;
  ~SplitViewLocationBarModelDelegateUnitTest() override = default;

 private:
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(SplitViewLocationBarModelDelegateUnitTest, ShouldDisplayURL_NewTabPage) {
  TestingProfile testing_profile;
  content::TestWebContentsFactory web_contents_factory;
  content::WebContents* web_contents =
      web_contents_factory.CreateWebContents(&testing_profile);
  web_contents->GetController().LoadURL(GURL("chrome://newtab/"), {}, {}, {});
  content::WebContentsTester::For(web_contents)->CommitPendingNavigation();
  ASSERT_EQ("chrome://newtab/", web_contents->GetURL().spec());

  // We want split view location bar to show new tab page's url
  SplitViewLocationBarModelDelegate delegate;
  delegate.set_web_contents(web_contents);
  EXPECT_TRUE(delegate.ShouldDisplayURL());
}
