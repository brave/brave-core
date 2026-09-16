/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snaps_service.h"

#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/snap/snap_request_handler_impl.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
constexpr char kTestSnapId[] = "npm:test-snap";
}  // namespace

class SnapsServiceBrowserTest : public InProcessBrowserTest {
 public:
  SnapsServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveWalletSnapsFeature);
  }

  void SetUpOnMainThread() override {
    auto* wallet_service = BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
    ASSERT_TRUE(wallet_service);
    service_ = wallet_service->snaps_service();
    ASSERT_TRUE(service_);
    service_->SetSnapBundleForTesting(kTestSnapId, ReadTestSnapBundle());
  }

  void OpenWalletPage() {
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return service()->IsBridgeBoundForTesting(); }));
  }

  std::pair<std::optional<std::string>, std::optional<std::string>> InvokeSnap(
      const std::string& method,
      const std::string& params_json) {
    base::test::TestFuture<std::optional<std::string>,
                           std::optional<std::string>>
        future;
    service_->InvokeSnap(
        kTestSnapId, method, params_json,
        future.GetCallback<const std::optional<std::string>&,
                           const std::optional<std::string>&>());
    return future.Take();
  }

  SnapsService* service() { return service_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  raw_ptr<SnapsService> service_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest,
                       InvokeSnapFailsWhenWalletPageIsNotRunning) {
  auto [result_json, error] = InvokeSnap("echo", R"({"hello":"world"})");
  EXPECT_FALSE(result_json.has_value());
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet page is not running", *error);
}

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest, InvokeSnapReachesSnap) {
  OpenWalletPage();

  auto [result_json, error] = InvokeSnap("echo", R"({"hello":"world"})");
  ASSERT_FALSE(error.has_value()) << *error;
  ASSERT_TRUE(result_json.has_value());

  auto parsed = base::JSONReader::Read(*result_json, base::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed);
  auto expected = base::JSONReader::Read(R"({"echoed":{"hello":"world"}})",
                                         base::JSON_PARSE_RFC);
  ASSERT_TRUE(expected);
  EXPECT_EQ(*parsed, *expected);
}

// A snap can invoke commands back so the browser side receives them, and the
// reply travels back into the snap.
IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest,
                       SnapRequestReachesRequestHandler) {
  OpenWalletPage();

  std::vector<std::tuple<std::string, std::string, base::Value>> observed;
  service()->snap_request_handler_for_testing()->SetRequestObserverForTesting(
      base::BindRepeating(
          [](std::vector<std::tuple<std::string, std::string, base::Value>>*
                 observed,
             const std::string& snap_id, const std::string& method,
             const base::Value& params) {
            observed->emplace_back(snap_id, method, params.Clone());
          },
          &observed));

  auto [result_json, error] = InvokeSnap("roundTrip", R"({"n":7})");
  ASSERT_FALSE(error.has_value()) << *error;
  ASSERT_TRUE(result_json.has_value());

  ASSERT_EQ(2u, observed.size());
  EXPECT_EQ(kTestSnapId, std::get<0>(observed[0]));
  EXPECT_EQ("snap_manageState", std::get<1>(observed[0]));
  EXPECT_EQ("update",
            *std::get<2>(observed[0]).GetDict().FindString("operation"));
  EXPECT_EQ(kTestSnapId, std::get<0>(observed[1]));
  EXPECT_EQ("snap_manageState", std::get<1>(observed[1]));
  EXPECT_EQ("get", *std::get<2>(observed[1]).GetDict().FindString("operation"));

  auto parsed = base::JSONReader::Read(*result_json, base::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed);
  auto expected = base::JSONReader::Read(R"({"fromBrowser":{"seen":{"n":7}}})",
                                         base::JSON_PARSE_RFC);
  ASSERT_TRUE(expected);
  EXPECT_EQ(*parsed, *expected);
}

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest, UnknownSnapReturnsError) {
  OpenWalletPage();

  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>>
      future;
  service()->InvokeSnap(
      "npm:missing", "echo", "{}",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  auto [result_json, error] = future.Take();
  EXPECT_FALSE(result_json.has_value());
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Bundle not found", *error);
}

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest, InvalidParamsJson) {
  auto [result_json, error] = InvokeSnap("echo", "not json");
  EXPECT_FALSE(result_json.has_value());
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Invalid params JSON", *error);
}

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest, BridgeDisconnectOnPageClose) {
  OpenWalletPage();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab")));
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !service()->IsBridgeBoundForTesting(); }));

  auto [result_json, error] = InvokeSnap("echo", R"({"hello":"world"})");
  EXPECT_FALSE(result_json.has_value());
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet page is not running", *error);
}

}  // namespace brave_wallet
