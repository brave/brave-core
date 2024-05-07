/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_rewards/common/pref_registry.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {

std::string FakeEncryption::EncryptString(const std::string& value) {
  return "ENCRYPTED:" + value;
}

std::optional<std::string> FakeEncryption::DecryptString(
    const std::string& value) {
  size_t pos = value.find("ENCRYPTED:");
  if (pos == 0) {
    return value.substr(10);
  }

  return {};
}

std::string FakeEncryption::Base64EncryptString(const std::string& value) {
  return base::Base64Encode(EncryptString(value));
}

std::optional<std::string> FakeEncryption::Base64DecryptString(
    const std::string& value) {
  std::string decoded;
  if (!base::Base64Decode(value, &decoded)) {
    return {};
  }

  return DecryptString(decoded);
}

TestNetworkResult::TestNetworkResult(const std::string& url,
                                     mojom::UrlMethod method,
                                     mojom::UrlResponsePtr response)
    : url(url), method(method), response(std::move(response)) {}

TestNetworkResult::~TestNetworkResult() = default;

base::FilePath GetTestDataPath() {
  base::FilePath path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);
  path = path.AppendASCII("brave");
  path = path.AppendASCII("components");
  path = path.AppendASCII("brave_rewards");
  path = path.AppendASCII("core");
  path = path.AppendASCII("test");
  path = path.AppendASCII("data");
  return path;
}

TestSPLAccountBalanceResult::TestSPLAccountBalanceResult(
    const std::string& solana_address,
    const std::string& token_mint_address,
    mojom::SolanaAccountBalancePtr balance)
    : solana_address(solana_address),
      token_mint_address(token_mint_address),
      balance(std::move(balance)) {}

TestSPLAccountBalanceResult::~TestSPLAccountBalanceResult() = default;

TestRewardsEngineClient::TestRewardsEngineClient()
    : engine_database_(base::FilePath()) {
  CHECK(engine_database_.GetInternalDatabaseForTesting()->OpenInMemory());
  brave_rewards::RegisterProfilePrefs(prefs_.registry());
}

TestRewardsEngineClient::~TestRewardsEngineClient() = default;

void TestRewardsEngineClient::OnReconcileComplete(
    mojom::Result result,
    mojom::ContributionInfoPtr contribution) {}

void TestRewardsEngineClient::OnPanelPublisherInfo(
    mojom::Result result,
    mojom::PublisherInfoPtr publisher_info,
    uint64_t window_id) {}

void TestRewardsEngineClient::FetchFavIcon(const std::string& url,
                                           const std::string& favicon_key,
                                           FetchFavIconCallback callback) {
  std::move(callback).Run(true, favicon_key);
}

void TestRewardsEngineClient::LoadURL(mojom::UrlRequestPtr request,
                                      LoadURLCallback callback) {
  DCHECK(request);
  auto iter = base::ranges::find_if(network_results_, [&request](auto& result) {
    return request->url == result.url && request->method == result.method;
  });

  if (iter != network_results_.end()) {
    std::move(callback).Run(std::move(iter->response));
    network_results_.erase(iter);
    return;
  }

  LOG(INFO) << "Test network result not found for " << request->method << ":"
            << request->url;

  auto response = mojom::UrlResponse::New();
  response->url = request->url;
  response->status_code = net::HTTP_BAD_REQUEST;
  std::move(callback).Run(std::move(response));
}

void TestRewardsEngineClient::GetSPLTokenAccountBalance(
    const std::string& solana_address,
    const std::string& token_mint_address,
    GetSPLTokenAccountBalanceCallback callback) {
  auto iter = base::ranges::find_if(spl_balance_results_, [&](auto& result) {
    return solana_address == result.solana_address &&
           token_mint_address == result.token_mint_address;
  });

  if (iter != spl_balance_results_.end()) {
    std::move(callback).Run(std::move(iter->balance));
    spl_balance_results_.erase(iter);
    return;
  }

  LOG(INFO) << "Test SPL token account balance result not found for "
            << solana_address;

  std::move(callback).Run(nullptr);
}

void TestRewardsEngineClient::PublisherListNormalized(
    std::vector<mojom::PublisherInfoPtr> list) {}

void TestRewardsEngineClient::OnPublisherRegistryUpdated() {}

void TestRewardsEngineClient::OnPublisherUpdated(
    const std::string& publisher_id) {}

void TestRewardsEngineClient::GetUserPreferenceValue(
    const std::string& path,
    GetUserPreferenceValueCallback callback) {
  return std::move(callback).Run(prefs_.GetValue(path).Clone());
}

void TestRewardsEngineClient::SetUserPreferenceValue(
    const std::string& path,
    base::Value value,
    SetUserPreferenceValueCallback callback) {
  prefs_.Set(path, std::move(value));
  std::move(callback).Run();
}

void TestRewardsEngineClient::ClearUserPreferenceValue(
    const std::string& path,
    ClearUserPreferenceValueCallback callback) {
  prefs_.ClearPref(path);
  std::move(callback).Run();
}

void TestRewardsEngineClient::ShowNotification(
    const std::string& type,
    const std::vector<std::string>& args,
    ShowNotificationCallback callback) {}

void TestRewardsEngineClient::ReconcileStampReset() {}

void TestRewardsEngineClient::RunDBTransaction(
    mojom::DBTransactionPtr transaction,
    RunDBTransactionCallback callback) {
  auto response = engine_database_.RunTransaction(std::move(transaction));
  std::move(callback).Run(std::move(response));
}

void TestRewardsEngineClient::Log(const std::string& file,
                                  int32_t line,
                                  int32_t verbose_level,
                                  const std::string& message) {
  int vlog_level =
      logging::GetVlogLevelHelper(file.c_str(), strlen(file.c_str()));

  if (verbose_level <= vlog_level) {
    logging::LogMessage(file.c_str(), line, -verbose_level).stream() << message;
  }
}

void TestRewardsEngineClient::ClearAllNotifications() {}

void TestRewardsEngineClient::ExternalWalletConnected() {
  observer_events_.push_back("external-wallet-connected");
}

void TestRewardsEngineClient::ExternalWalletLoggedOut() {
  observer_events_.push_back("external-wallet-logged-out");
}

void TestRewardsEngineClient::ExternalWalletReconnected() {
  observer_events_.push_back("external-wallet-reconnected");
}

void TestRewardsEngineClient::ExternalWalletDisconnected() {
  observer_events_.push_back("external-wallet-disconnected");
}

void TestRewardsEngineClient::DeleteLog(DeleteLogCallback callback) {
  std::move(callback).Run(mojom::Result::OK);
}

void TestRewardsEngineClient::EncryptString(const std::string& value,
                                            EncryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::EncryptString(value));
}

void TestRewardsEngineClient::DecryptString(const std::string& value,
                                            DecryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::DecryptString(value));
}

void TestRewardsEngineClient::AddNetworkResultForTesting(
    const std::string& url,
    mojom::UrlMethod method,
    mojom::UrlResponsePtr response) {
  network_results_.emplace_back(url, method, std::move(response));
}

void TestRewardsEngineClient::AddSPLAccountBalanceResultForTesting(
    const std::string& solana_address,
    const std::string& token_mint_address,
    mojom::SolanaAccountBalancePtr balance) {
  spl_balance_results_.emplace_back(solana_address, token_mint_address,
                                    std::move(balance));
}

}  // namespace brave_rewards::internal
