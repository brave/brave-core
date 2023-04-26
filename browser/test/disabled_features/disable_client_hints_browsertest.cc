/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <functional>
#include <set>
#include <vector>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
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
const char kNoClientHintsHeaders[] = "/simple.html";
const char kClientHints[] = "/ch.html";
const char kClientHintsDelegationMerge[] = "/ch_delegation_merge.html";
const char KClientHintsMetaHTTPEquivAcceptCH[] =
    "/ch-meta-http-equiv-accept-ch.html";
const char KClientHintsMetaNameAcceptCH[] = "/ch-meta-name-accept-ch.html";

const char kPlatformVersionClientHintPatchValue[] = "x";

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

    no_client_hints_headers_url_ = https_server_.GetURL(kNoClientHintsHeaders);
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
    std::vector<base::test::FeatureRefAndParams> enabled_features;
    std::vector<base::test::FeatureRef> disabled_features;
    for (const auto& feature : kTestFeatures) {
      if (IsClientHintHeaderEnabled()) {
        enabled_features.emplace_back(feature.get(), base::FieldTrialParams());
      } else {
        disabled_features.push_back(feature.get());
      }
    }

    if (IsBraveClientHintFeatureEnabled()) {
      enabled_features.emplace_back(blink::features::kAllowCertainClientHints,
                                    base::FieldTrialParams());
      base::FieldTrialParams parameters;
      parameters[blink::features::kClampPlatformVersionClientHintPatchValue
                     .name] = kPlatformVersionClientHintPatchValue;
      enabled_features.emplace_back(
          blink::features::kClampPlatformVersionClientHint, parameters);
    } else {
      disabled_features.push_back(blink::features::kAllowCertainClientHints);
    }

    scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                       disabled_features);

    if (IsBraveClientHintFeatureEnabled()) {
      PopulateDefaultClientHints();
      PopulateAllowedClientHints();
    }
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    base::RunLoop().RunUntilIdle();
  }

  void TearDownOnMainThread() override {}

  const GURL& no_client_hints_headers_url() const {
    return no_client_hints_headers_url_;
  }

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

  size_t default_client_hints_headers_seen_count() const {
    return default_client_hints_headers_seen_.size();
  }

  size_t allowed_client_hints_headers_seen_count() const {
    return allowed_client_hints_headers_seen_.size();
  }

  size_t client_hints_headers_seen_count() const {
    return unexpected_client_hints_headers_seen_.size();
  }

  std::string default_client_hints_headers_seen() const {
    return base::JoinString(
        std::vector<std::string>(default_client_hints_headers_seen_.begin(),
                                 default_client_hints_headers_seen_.end()),
        ", ");
  }

  std::string allowed_client_hints_headers_seen() const {
    return base::JoinString(
        std::vector<std::string>(allowed_client_hints_headers_seen_.begin(),
                                 allowed_client_hints_headers_seen_.end()),
        ", ");
  }

  std::string unexpected_client_hints_headers_seen() const {
    return base::JoinString(unexpected_client_hints_headers_seen_, ", ");
  }

  std::string platform_version_client_hint_value() const {
    return platform_version_client_hint_value_;
  }

  void reset_client_hints_headers_seen() {
    default_client_hints_headers_seen_.clear();
    allowed_client_hints_headers_seen_.clear();
    unexpected_client_hints_headers_seen_.clear();
    platform_version_client_hint_value_ = "";
  }

  bool VerifyPlatformVersionClientHintPatchValue() const {
    return base::EndsWith(
        platform_version_client_hint_value_,
        base::StrCat({".", kPlatformVersionClientHintPatchValue, "\""}));
  }

 private:
  void PopulateDefaultClientHints() {
    const auto& hints_map = network::GetClientHintToNameMap();
    default_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUA));
    default_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAMobile));
    default_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAPlatform));
  }

  void PopulateAllowedClientHints() {
    const auto& hints_map = network::GetClientHintToNameMap();
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAModel));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAPlatformVersion));
  }

  bool IsPlatformVersionClientHintHeader(const std::string& header) const {
    return header ==
           network::GetClientHintToNameMap().at(
               network::mojom::WebClientHintsType::kUAPlatformVersion);
  }

  void StorePlatformVersionClientHintHeaderValue(
      const net::test_server::HttpRequest::HeaderMap& headers) {
    const auto& hints_map = network::GetClientHintToNameMap();
    const std::string& platform_version_header_name =
        hints_map.at(network::mojom::WebClientHintsType::kUAPlatformVersion);
    platform_version_client_hint_value_ =
        headers.at(platform_version_header_name);
  }

  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    for (const auto& elem : network::GetClientHintToNameMap()) {
      const auto& header = elem.second;
      if (base::Contains(request.headers, header)) {
        if (IsBraveClientHintFeatureEnabled()) {
          if (base::Contains(default_hints_, header)) {
            default_client_hints_headers_seen_.insert(header);
            continue;
          } else if (base::Contains(allowed_hints_, header)) {
            allowed_client_hints_headers_seen_.insert(header);
            if (IsPlatformVersionClientHintHeader(header)) {
              StorePlatformVersionClientHintHeaderValue(request.headers);
            }
            continue;
          }
        }
        unexpected_client_hints_headers_seen_.push_back(header);
      }
    }
  }

  net::EmbeddedTestServer https_server_;

  GURL no_client_hints_headers_url_;
  GURL client_hints_delegation_merge_url_;
  GURL client_hints_meta_http_equiv_accept_ch_url_;
  GURL client_hints_meta_name_accept_ch_url_;
  GURL client_hints_url_;

  std::set<std::string> default_client_hints_headers_seen_;
  std::set<std::string> allowed_client_hints_headers_seen_;
  std::vector<std::string> unexpected_client_hints_headers_seen_;

  std::vector<std::string> default_hints_;
  std::vector<std::string> allowed_hints_;

  std::string platform_version_client_hint_value_;

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(ClientHintsBrowserTest, CheckClientHints) {
  for (const auto& feature : kTestFeatures) {
    EXPECT_EQ(IsClientHintHeaderEnabled(),
              base::FeatureList::IsEnabled(feature));
  }
  EXPECT_EQ(
      IsBraveClientHintFeatureEnabled(),
      base::FeatureList::IsEnabled(blink::features::kAllowCertainClientHints));

  const size_t expected_default_client_hints_count =
      IsClientHintHeaderEnabled() && IsBraveClientHintFeatureEnabled() ? 3u
                                                                       : 0u;
  const size_t expected_allowed_client_hints_count =
      IsClientHintHeaderEnabled() && IsBraveClientHintFeatureEnabled() ? 2u
                                                                       : 0u;

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), no_client_hints_headers_url()));
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(0u, allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), client_hints_url()));
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();
  if (IsClientHintHeaderEnabled() && IsBraveClientHintFeatureEnabled()) {
    EXPECT_TRUE(VerifyPlatformVersionClientHintPatchValue())
        << "Expected the patch field value to be: '"
        << kPlatformVersionClientHintPatchValue << "'. "
        << "Actual platform version value: "
        << platform_version_client_hint_value();
  }

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_meta_http_equiv_accept_ch_url()));
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_meta_name_accept_ch_url()));
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_delegation_merge_url()));
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();
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
