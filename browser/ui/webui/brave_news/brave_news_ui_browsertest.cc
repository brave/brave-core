// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class BraveNewsUIBrowserTest : public InProcessBrowserTest {
 public:
  BraveNewsUIBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        brave_news::features::kBraveNewsSidebar);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));
    embedded_https_test_server().ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_https_test_server().Start());
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// chrome-untrusted://brave-image must allow the Brave News page, as
// WebUIURLLoaderFactory rejects cross-origin loads between chrome-untrusted://
// hosts otherwise.
IN_PROC_BROWSER_TEST_F(BraveNewsUIBrowserTest, LoadsBraveImage) {
  content::WebContents* web_contents =
      chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(content::NavigateToURL(web_contents, GURL(kBraveNewsURL)));

  constexpr char kLoadImageScript[] = R"(
    new Promise(resolve => {
      const img = new Image();
      img.onload = () => resolve('loaded');
      img.onerror = () => resolve('error');
      img.src = $1;
    });
  )";
  const GURL image_url(
      base::StrCat({"chrome-untrusted://brave-image?url=",
                    base::EscapeQueryParamValue(
                        embedded_https_test_server().GetURL("/logo.png").spec(),
                        /*use_plus=*/false)}));
  EXPECT_EQ("loaded",
            content::EvalJs(web_contents,
                            content::JsReplace(kLoadImageScript, image_url)));
}
