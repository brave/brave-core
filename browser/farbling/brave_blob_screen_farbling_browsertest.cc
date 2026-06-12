/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/features.h"

using brave_shields::ControlType;
using content::TitleWatcher;

namespace {

constexpr gfx::Rect kTestWindowBounds(100, 100, 400, 400);

}  // namespace

class BraveBlobScreenFarblingBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  BraveBlobScreenFarblingBrowserTest() {
    if (GetParam()) {
      feature_list_.InitAndEnableFeature(
          blink::features::kBraveBlockScreenFingerprinting);
    } else {
      feature_list_.InitAndDisableFeature(
          blink::features::kBraveBlockScreenFingerprinting);
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());
    embedded_test_server()->ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    ASSERT_TRUE(embedded_test_server()->Start());

    blob_test_url_ =
        embedded_test_server()->GetURL("a.com", "/blob-fingerprinting.html");
  }

  HostContentSettingsMap* ContentSettings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void SetFingerprintingSetting(bool allow) {
    brave_shields::SetFingerprintingControlType(
        ContentSettings(), allow ? ControlType::ALLOW : ControlType::DEFAULT,
        blob_test_url_);
  }

  content::WebContents* Contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* Parent() const {
    return Contents()->GetPrimaryMainFrame();
  }

  // The driver of test is basically on the JS side which simplifies testing a
  // lot. See brave_screen_farbling_browsertest for comparison.
  // The JS test basically does the following:
  // 1. Reads the various screen attributes from the parent window and writes to
  // the localStorage keyed under parent_.
  // 2. Opens a blob:// window.
  // 3. Reads the various screen attributes from the blob window and then writes
  // to the localStorage keyed under blob_.
  // 4. Compares the two and they should match for passing tests.
  // 4.1) For cases where fingerprinting was blocked: both the parent and the
  // blob window sees the same "farbled" value. 4.2) For cases where
  // fingerprinting was not blocked: both the parent and the blob window sees
  // the same "un-farbled" value.
  void FarbleScreenBlobURL() {
    ui_test_utils::SetAndWaitForBounds(*browser(), kTestWindowBounds);

    SetFingerprintingSetting(/*allow=*/false);
    // TODO(https://github.com/brave/brave-browser/issues/56048): Expect pass
    // once Shields are supported on blob:// URLs.
    NavigateToBlob(GetParam() ? u"fail" : u"pass");

    SetFingerprintingSetting(/*allow=*/true);
    NavigateToBlob(u"pass");
  }

  void NavigateToBlob(const std::u16string& expected_title) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), blob_test_url_));
    content::ExecuteScriptAsync(Parent(),
                                "startBlobScreenFingerprintingTest()");
    Browser* popup = ui_test_utils::WaitForBrowserToOpen();
    EXPECT_NE(popup, browser());
    auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();

    EXPECT_TRUE(WaitForRenderFrameReady(popup_contents->GetPrimaryMainFrame()));
    TitleWatcher watcher(popup_contents, expected_title);
    EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
    CloseBrowserSynchronously(popup);
  }

  // Similar to FarbleScreenBlobURL but renders the blob in an iframe.
  void FarbleScreenBlobURLIframe() {
    ui_test_utils::SetAndWaitForBounds(*browser(), kTestWindowBounds);

    SetFingerprintingSetting(/*allow=*/false);
    NavigateToBlobIframe();

    SetFingerprintingSetting(/*allow=*/true);
    NavigateToBlobIframe();
  }

  void NavigateToBlobIframe() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), blob_test_url_));
    content::ExecuteScriptAsync(Parent(),
                                "startBlobScreenFingerprintingIframeTest()");
    TitleWatcher watcher(Contents(), u"pass");
    EXPECT_EQ(u"pass", watcher.WaitAndGetTitle());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  GURL blob_test_url_;
};

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveBlobScreenFarblingBrowserTest,
    testing::Bool(),
    [](const testing::TestParamInfo<bool>& info) {
      return info.param ? "BraveBlockScreenFingerprinting_Enabled"
                        : "BraveBlockScreenFingerprinting_Disabled";
    });

IN_PROC_BROWSER_TEST_P(BraveBlobScreenFarblingBrowserTest,
                       FarbleScreenBlobURL) {
  FarbleScreenBlobURL();
}

IN_PROC_BROWSER_TEST_P(BraveBlobScreenFarblingBrowserTest,
                       FarbleScreenBlobURLIframe) {
  FarbleScreenBlobURLIframe();
}
