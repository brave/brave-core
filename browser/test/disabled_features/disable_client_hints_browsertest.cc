/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <functional>
#include <vector>

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "services/network/public/cpp/client_hints.h"
#include "services/network/public/mojom/web_client_hints_types.mojom-shared.h"
#include "third_party/blink/public/common/client_hints/client_hints.h"
#include "third_party/blink/public/common/features.h"

namespace {
const char kClientHints[] = "/ch.html";
const char kClientHintsDelegationMerge[] = "/ch_delegation_merge.html";
const char KClientHintsMetaHTTPEquivAcceptCH[] =
    "/ch-meta-http-equiv-accept-ch.html";
const char KClientHintsMetaNameAcceptCH[] = "/ch-meta-name-accept-ch.html";

const std::reference_wrapper<const base::Feature> kTestFeatures[] = {
    // Individual hints features
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
    // Client hints features
    blink::features::kClientHintsMetaHTTPEquivAcceptCH,
    blink::features::kClientHintThirdPartyDelegation,
};

}  // namespace

class ClientHintsBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<std::tuple<bool, bool>> {
 public:
  ClientHintsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.RegisterRequestMonitor(
        base::BindRepeating(&ClientHintsBrowserTest::MonitorResourceRequest,
                            base::Unretained(this)));

    EXPECT_TRUE(https_server_.Start());

    client_hints_url_ = https_server_.GetURL(kClientHints);
    client_hints_delegation_merge_url_ =
        https_server_.GetURL(kClientHintsDelegationMerge);
    client_hints_meta_http_equiv_accept_ch_url_ =
        https_server_.GetURL(KClientHintsMetaHTTPEquivAcceptCH);
    client_hints_meta_name_accept_ch_url_ =
        https_server_.GetURL(KClientHintsMetaNameAcceptCH);
  }

  ClientHintsBrowserTest(const ClientHintsBrowserTest&) = delete;

  ClientHintsBrowserTest& operator=(const ClientHintsBrowserTest&) = delete;

  ~ClientHintsBrowserTest() override = default;

  bool IsClientHintHeaderEnabled() { return std::get<0>(GetParam()); }
  bool IsBraveClientHintFeatureEnabled() { return std::get<1>(GetParam()); }

  void SetUp() override {
    // Test that even with CH features enabled, there is no header.
    std::vector<base::test::FeatureRef> enabled_features;
    std::vector<base::test::FeatureRef> disabled_features;
    for (const auto& feature : kTestFeatures) {
      if (IsClientHintHeaderEnabled()) {
        enabled_features.push_back(feature.get());
      } else {
        disabled_features.push_back(feature.get());
      }
    }

    if (IsBraveClientHintFeatureEnabled()) {
      enabled_features.push_back(blink::features::kAllowCertainClientHints);
    } else {
      disabled_features.push_back(blink::features::kAllowCertainClientHints);
    }

    scoped_feature_list_.InitWithFeatures(enabled_features, disabled_features);

    if (IsBraveClientHintFeatureEnabled()) {
      PopulateAllowedClientHints();
    }
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    base::RunLoop().RunUntilIdle();
  }

  void TearDownOnMainThread() override {}

  const GURL& client_hints_url() const { return client_hints_url_; }

  const GURL& client_hints_delegation_merge_url() const {
    return client_hints_delegation_merge_url_;
  }

  const GURL& client_hints_meta_http_equiv_accept_ch_url() const {
    return client_hints_meta_http_equiv_accept_ch_url_;
  }
  const GURL& client_hints_meta_name_accept_ch_url() const {
    return client_hints_meta_name_accept_ch_url_;
  }

  size_t count_client_hints_headers_seen() const {
    return count_client_hints_headers_seen_;
  }

  void reset_client_hints_headers_seen_count() {
    count_client_hints_headers_seen_ = 0;
  }

 private:
  void PopulateAllowedClientHints() {
    const auto& hints_map = network::GetClientHintToNameMap();
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUA));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAMobile));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAPlatform));
  }

  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    for (const auto& elem : network::GetClientHintToNameMap()) {
      const auto& header = elem.second;
      if (base::Contains(request.headers, header)) {
        if (IsBraveClientHintFeatureEnabled() &&
            base::Contains(allowed_hints_, header)) {
          continue;
        }
        count_client_hints_headers_seen_++;
      }
    }
  }

  net::EmbeddedTestServer https_server_;
  GURL client_hints_delegation_merge_url_;
  GURL client_hints_meta_http_equiv_accept_ch_url_;
  GURL client_hints_meta_name_accept_ch_url_;
  GURL client_hints_url_;
  size_t count_client_hints_headers_seen_ = 0;
  std::vector<std::string> allowed_hints_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(ClientHintsBrowserTest, ClientHintsDisabled) {
  for (const auto& feature : kTestFeatures) {
    EXPECT_EQ(IsClientHintHeaderEnabled(),
              base::FeatureList::IsEnabled(feature));
  }
  EXPECT_EQ(
      IsBraveClientHintFeatureEnabled(),
      base::FeatureList::IsEnabled(blink::features::kAllowCertainClientHints));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), client_hints_url()));
  EXPECT_EQ(0u, count_client_hints_headers_seen());

  reset_client_hints_headers_seen_count();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_meta_http_equiv_accept_ch_url()));
  EXPECT_EQ(0u, count_client_hints_headers_seen());

  reset_client_hints_headers_seen_count();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_meta_name_accept_ch_url()));
  EXPECT_EQ(0u, count_client_hints_headers_seen());

  reset_client_hints_headers_seen_count();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_delegation_merge_url()));
  EXPECT_EQ(0u, count_client_hints_headers_seen());
}

INSTANTIATE_TEST_SUITE_P(
    ClientHintsBrowserTest,
    ClientHintsBrowserTest,
    ::testing::Combine(::testing::Bool(), ::testing::Bool()),
    [](const testing::TestParamInfo<ClientHintsBrowserTest::ParamType>& info) {
      bool chromium_features_enabled = std::get<0>(info.param);
      bool brave_feature_enabled = std::get<1>(info.param);
      return base::StringPrintf(
          "ChromiumCHFeatures%s_BraveCHFeature%s",
          chromium_features_enabled ? "Enabled" : "Disabled",
          brave_feature_enabled ? "Enabled" : "Disabled");
    });
