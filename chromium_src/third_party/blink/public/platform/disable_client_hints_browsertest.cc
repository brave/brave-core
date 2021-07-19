/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/common/brave_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/client_hints/client_hints.h"

const char kClientHints[] = "/ch.html";

class ClientHintsBrowserTest : public InProcessBrowserTest,
                               public ::testing::WithParamInterface<bool> {
 public:
  ClientHintsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS),
        count_client_hints_headers_seen_(0) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.RegisterRequestMonitor(
        base::BindRepeating(&ClientHintsBrowserTest::MonitorResourceRequest,
                            base::Unretained(this)));

    EXPECT_TRUE(https_server_.Start());

    client_hints_url_ = https_server_.GetURL(kClientHints);
  }

  ~ClientHintsBrowserTest() override {}

  bool IsLangClientHintHeaderEnabled() { return GetParam(); }

  void SetUp() override {
    if (IsLangClientHintHeaderEnabled()) {
      // Test that even with Lang CH feature enabled, there is no header.
      scoped_feature_list_.InitAndEnableFeature(
          features::kLangClientHintHeader);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          features::kLangClientHintHeader);
    }
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    base::RunLoop().RunUntilIdle();
  }

  void TearDownOnMainThread() override {}

  const GURL& client_hints_url() const { return client_hints_url_; }

  size_t count_client_hints_headers_seen() const {
    return count_client_hints_headers_seen_;
  }

 private:
  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    for (size_t i = 0; i < blink::kClientHintsMappingsCount; ++i) {
      if (base::Contains(request.headers,
                         blink::kClientHintsHeaderMapping[i])) {
        count_client_hints_headers_seen_++;
      }
    }
  }

  net::EmbeddedTestServer https_server_;
  GURL client_hints_url_;
  size_t count_client_hints_headers_seen_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ClientHintsBrowserTest);
};

IN_PROC_BROWSER_TEST_P(ClientHintsBrowserTest, ClientHintsDisabled) {
  EXPECT_EQ(IsLangClientHintHeaderEnabled(),
            base::FeatureList::IsEnabled(features::kLangClientHintHeader));
  ui_test_utils::NavigateToURL(browser(), client_hints_url());
  EXPECT_EQ(0u, count_client_hints_headers_seen());
}

INSTANTIATE_TEST_SUITE_P(ClientHintsBrowserTest,
                         ClientHintsBrowserTest,
                         ::testing::Bool());
