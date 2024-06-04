/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class FontBrowserTest : public InProcessBrowserTest {
 public:
  FontBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~FontBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(FontBrowserTest, FreetypeRegression) {
  // Loading this page on 1.50.121 on Linux crashed the tab.
  // See https://github.com/brave/brave-browser/issues/29893
  // and https://bugs.chromium.org/p/chromium/issues/detail?id=1434194
  const GURL url = https_server_.GetURL("/freetype-crash.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
}
