// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class UntrustedSanitizedImageSourceBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));
    embedded_https_test_server().ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_https_test_server().Start());
  }
};

// chrome-untrusted://image must allow the pages that load from it, as
// WebUIURLLoaderFactory rejects cross-origin loads between chrome-untrusted://
// hosts otherwise.
IN_PROC_BROWSER_TEST_F(UntrustedSanitizedImageSourceBrowserTest,
                       WalletPagesLoadImages) {
  constexpr char kLoadImageScript[] = R"(
    new Promise(resolve => {
      const img = new Image();
      img.onload = () => resolve('loaded');
      img.onerror = () => resolve('error');
      img.src = $1;
    });
  )";
  const GURL image_url(
      base::StrCat({"chrome-untrusted://image?url=",
                    base::EscapeQueryParamValue(
                        embedded_https_test_server().GetURL("/logo.png").spec(),
                        /*use_plus=*/false)}));
  content::WebContents* web_contents =
      chrome_test_utils::GetActiveWebContents(this);
  for (const char* page_url : {kUntrustedNftURL, kUntrustedMarketURL}) {
    SCOPED_TRACE(page_url);
    ASSERT_TRUE(content::NavigateToURL(web_contents, GURL(page_url)));
    EXPECT_EQ("loaded",
              content::EvalJs(web_contents,
                              content::JsReplace(kLoadImageScript, image_url)));
  }
}
