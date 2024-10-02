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
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "services/network/public/cpp/client_hints.h"
#include "services/network/public/mojom/web_client_hints_types.mojom-shared.h"
#include "third_party/blink/public/common/features.h"

namespace {
constexpr char kNoClientHintsHeaders[] = "/simple.html";
constexpr char kClientHints[] = "/ch.html";
constexpr char kClientHintsDelegationMerge[] = "/ch_delegation_merge.html";
constexpr char kClientHintsMetaHTTPEquivAcceptCH[] =
    "/ch-meta-http-equiv-accept-ch.html";
constexpr char kClientHintsMetaNameAcceptCH[] = "/ch-meta-name-accept-ch.html";

const std::reference_wrapper<const base::Feature> kTestFeatures[] = {
    // Individual hints features
    blink::features::kClientHintsDeviceMemory_DEPRECATED,
    blink::features::kClientHintsDPR_DEPRECATED,
    blink::features::kClientHintsResourceWidth_DEPRECATED,
    blink::features::kClientHintsViewportWidth_DEPRECATED,
    blink::features::kViewportHeightClientHintHeader,
};

}  // namespace

class ClientHintsBrowserTest : public InProcessBrowserTest,
                               public ::testing::WithParamInterface<bool> {
 public:
  ClientHintsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  bool IsClientHintHeaderEnabled() { return GetParam(); }

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

    scoped_feature_list_.InitWithFeatures(enabled_features, disabled_features);

    PopulateDefaultClientHints();
    PopulateAllowedClientHints();
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.RegisterRequestMonitor(base::BindPostTaskToCurrentDefault(
        base::BindRepeating(&ClientHintsBrowserTest::MonitorResourceRequest,
                            weak_ptr_factory_.GetWeakPtr())));

    EXPECT_TRUE(https_server_.Start());

    no_client_hints_headers_url_ = https_server_.GetURL(kNoClientHintsHeaders);
    client_hints_url_ = https_server_.GetURL(kClientHints);
    client_hints_delegation_merge_url_ =
        https_server_.GetURL(kClientHintsDelegationMerge);
    client_hints_meta_http_equiv_accept_ch_url_ =
        https_server_.GetURL(kClientHintsMetaHTTPEquivAcceptCH);
    client_hints_meta_name_accept_ch_url_ =
        https_server_.GetURL(kClientHintsMetaNameAcceptCH);

    host_resolver()->AddRule("*", "127.0.0.1");
    InProcessBrowserTest::SetUpOnMainThread();
  }

  void FlushPostedTasks() {
    base::RunLoop run_loop;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, run_loop.QuitClosure());
    run_loop.Run();
  }

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

  size_t unexpected_client_hints_headers_seen_count() const {
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

  void reset_client_hints_headers_seen() {
    default_client_hints_headers_seen_.clear();
    allowed_client_hints_headers_seen_.clear();
    unexpected_client_hints_headers_seen_.clear();
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
        hints_map.at(network::mojom::WebClientHintsType::kUAArch));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUABitness));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAFullVersionList));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAModel));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAPlatformVersion));
    allowed_hints_.push_back(
        hints_map.at(network::mojom::WebClientHintsType::kUAWoW64));
  }

  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    for (const auto& elem : network::GetClientHintToNameMap()) {
      const auto& header = elem.second;
      if (base::Contains(request.headers, header)) {
        if (base::Contains(default_hints_, header)) {
          default_client_hints_headers_seen_.insert(header);
          continue;
        } else if (base::Contains(allowed_hints_, header)) {
          allowed_client_hints_headers_seen_.insert(header);
          continue;
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

  base::test::ScopedFeatureList scoped_feature_list_;

  base::WeakPtrFactory<ClientHintsBrowserTest> weak_ptr_factory_{this};
};

IN_PROC_BROWSER_TEST_P(ClientHintsBrowserTest, ClientHintsDisabled) {
  for (const auto& feature : kTestFeatures) {
    EXPECT_EQ(IsClientHintHeaderEnabled(),
              base::FeatureList::IsEnabled(feature));
  }

  const size_t expected_default_client_hints_count = 3u;
  const size_t expected_allowed_client_hints_count = 6u;

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), no_client_hints_headers_url()));
  FlushPostedTasks();
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(0u, allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, unexpected_client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), client_hints_url()));
  FlushPostedTasks();
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, unexpected_client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_meta_http_equiv_accept_ch_url()));
  FlushPostedTasks();
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, unexpected_client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_meta_name_accept_ch_url()));
  FlushPostedTasks();
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, unexpected_client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();

  reset_client_hints_headers_seen();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), client_hints_delegation_merge_url()));
  FlushPostedTasks();
  EXPECT_EQ(expected_default_client_hints_count,
            default_client_hints_headers_seen_count())
      << "Default headers seen: " << default_client_hints_headers_seen();
  EXPECT_EQ(expected_allowed_client_hints_count,
            allowed_client_hints_headers_seen_count())
      << "Allowed headers seen: " << allowed_client_hints_headers_seen();
  EXPECT_EQ(0u, unexpected_client_hints_headers_seen_count())
      << "Unexpected headers: " << unexpected_client_hints_headers_seen();
}

INSTANTIATE_TEST_SUITE_P(
    ClientHintsBrowserTest,
    ClientHintsBrowserTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<ClientHintsBrowserTest::ParamType>& info) {
      return base::StringPrintf("ChromiumCHFeatures_%s",
                                info.param ? "Enabled" : "Disabled");
    });
