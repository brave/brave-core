// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

using BraveUniversalWebContentsObserversBrowserTest = PlatformBrowserTest;

// Note: This is a browsertest because we want to check that all the machinery
// is tied together properly.
IN_PROC_BROWSER_TEST_F(BraveUniversalWebContentsObserversBrowserTest,
                       CreatedWebContentsAddsUniversalWebContentsObservers) {
  content::WebContents::CreateParams params(GetProfile());
  params.initially_hidden = true;
  params.preview_mode = true;

  // Note: We don't create a tab here because we want to test that the observers
  // are added in the most minimal scenario (and without AttachTabHelpers being
  // called).
  auto web_contents = content::WebContents::Create(params);
  ASSERT_TRUE(web_contents);

  EXPECT_TRUE(brave_shields::BraveShieldsWebContentsObserver::FromWebContents(
      web_contents.get()));
  EXPECT_TRUE(ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
      web_contents.get()));
}
