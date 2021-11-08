/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <functional>

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
#include "services/network/public/cpp/client_hints.h"
#include "third_party/blink/public/common/client_hints/client_hints.h"
#include "third_party/blink/public/common/features.h"

namespace {
const char kClientHints[] = "/ch.html";
const std::reference_wrapper<const base::Feature> kTestFeatures[] = {
    blink::features::kClientHintsDeviceMemory,
    blink::features::kClientHintsDeviceMemory_DEPRECATED,
    blink::features::kClientHintsDPR,
    blink::features::kClientHintsDPR_DEPRECATED,
    blink::features::kClientHintsResourceWidth,
    blink::features::kClientHintsResourceWidth_DEPRECATED,
    blink::features::kClientHintsViewportWidth,
    blink::features::kClientHintsViewportWidth_DEPRECATED,
    blink::features::kPrefersColorSchemeClientHintHeader,
    blink::features::kUserAgentClientHint,
    blink::features::kViewportHeightClientHintHeader,
};
}  // namespace

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

  ClientHintsBrowserTest(const ClientHintsBrowserTest&) = delete;

  ClientHintsBrowserTest& operator=(const ClientHintsBrowserTest&) = delete;

  ~ClientHintsBrowserTest() override {}

  bool IsClientHintHeaderEnabled() { return GetParam(); }

  void SetUp() override {
    // Test that even with CH features enabled, there is no header.
    std::vector<base::Feature> enabled_features;
    std::vector<base::Feature> disabled_features;
    for (const auto& feature : kTestFeatures) {
      if (IsClientHintHeaderEnabled()) {
        enabled_features.push_back(feature);
      } else {
        disabled_features.push_back(feature);
      }
    }

    scoped_feature_list_.InitWithFeatures(enabled_features, disabled_features);
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
    for (const auto& elem : network::GetClientHintToNameMap()) {
      const auto& header = elem.second;
      if (base::Contains(request.headers, header)) {
        count_client_hints_headers_seen_++;
      }
    }
  }

  net::EmbeddedTestServer https_server_;
  GURL client_hints_url_;
  size_t count_client_hints_headers_seen_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(ClientHintsBrowserTest, ClientHintsDisabled) {
  for (const auto& feature : kTestFeatures) {
    EXPECT_EQ(IsClientHintHeaderEnabled(),
              base::FeatureList::IsEnabled(feature));
  }
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), client_hints_url()));
  EXPECT_EQ(0u, count_client_hints_headers_seen());
}

INSTANTIATE_TEST_SUITE_P(ClientHintsBrowserTest,
                         ClientHintsBrowserTest,
                         ::testing::Bool());
