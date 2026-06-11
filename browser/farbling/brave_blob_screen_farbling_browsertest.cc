/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/files/file_path.h"
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

constexpr char16_t kPassTitle[] = u"pass";
constexpr char16_t kFailTitle[] = u"fail";
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

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

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

  void FarbleScreenBlobURL() {
    ui_test_utils::SetAndWaitForBounds(*browser(), kTestWindowBounds);

    // Fingerprinting is blocked.
    SetFingerprintingSetting(/*allow=*/false);
    // TODO(https://github.com/brave/brave-browser/issues/56048): Expect pass
    // once Shields are supported on blob:// URLs.
    std::u16string expected_title = GetParam() ? kFailTitle : kPassTitle;
    NavigateToBlob(expected_title);

    // Regardless of the feature flag we should see the tests passing.
    SetFingerprintingSetting(/*allow=*/true);
    expected_title = kPassTitle;
    NavigateToBlob(expected_title);
  }

  void NavigateToBlob(const std::u16string& expected_title) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), blob_test_url_));
    content::ExecuteScriptAsync(Parent(),
                                "startBlobScreenFingerprintingTest()");
    Browser* popup = ui_test_utils::WaitForBrowserToOpen();
    auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
    TitleWatcher watcher(popup_contents, kPassTitle);
    watcher.AlsoWaitForTitle(kFailTitle);
    EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
    CloseBrowserSynchronously(popup);
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
