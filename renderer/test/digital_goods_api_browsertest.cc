/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class DigitalGoodsAPIBrowserTest : public InProcessBrowserTest,
                                   public ::testing::WithParamInterface<bool> {
 public:
  DigitalGoodsAPIBrowserTest() {
    if (GetParam()) {
      // Enable the DigitalGoodsApi base feature. This is needed because
      // Brave overrides kDigitalGoodsApi to FEATURE_DISABLED_BY_DEFAULT,
      // and this override causes the kSetOnlyIfOverridden logic in
      // SetRuntimeFeaturesFromChromiumFeatures() to disable the Blink
      // runtime feature even when --enable-blink-features=DigitalGoods
      // or --enable-blink-test-features is used.
      scoped_feature_list_.InitAndEnableFeature(features::kDigitalGoodsApi);
    }
  }

  ~DigitalGoodsAPIBrowserTest() override = default;

  bool IsDigitalGoodsAPIEnabled() { return GetParam(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    if (IsDigitalGoodsAPIEnabled()) {
#if BUILDFLAG(IS_ANDROID)
      command_line->AppendSwitch(
          switches::kEnableExperimentalWebPlatformFeatures);
#else
      command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                      "DigitalGoods");
#endif
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    // Use the default HTTP embedded test server. Navigating to 127.0.0.1
    // is considered a secure context (potentially trustworthy origin) per
    // the W3C Secure Contexts spec, so [SecureContext] APIs are available.
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* primary_main_frame() {
    return web_contents()->GetPrimaryMainFrame();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(DigitalGoodsAPIBrowserTest, DigitalGoods) {
  const GURL url = embedded_test_server()->GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto result =
      content::EvalJs(primary_main_frame(), "window.getDigitalGoodsService()");
  if (IsDigitalGoodsAPIEnabled()) {
    EXPECT_THAT(result,
                content::EvalJsResult::ErrorIs(testing::HasSubstr(
                    "1 argument required, but only 0 present.")));
  } else {
    EXPECT_THAT(result,
                content::EvalJsResult::ErrorIs(testing::HasSubstr(
                    "window.getDigitalGoodsService is not a function")));
  }
}

INSTANTIATE_TEST_SUITE_P(DigitalGoodsAPIBrowserTest,
                         DigitalGoodsAPIBrowserTest,
                         ::testing::Bool());
