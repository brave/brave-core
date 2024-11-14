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
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom-test-utils.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
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
}

TestRewardsEngineClient::~TestRewardsEngineClient() = default;

void TestRewardsEngineClient::LoadLegacyState(
    LoadLegacyStateCallback callback) {
  std::move(callback).Run(mojom::Result::NO_LEGACY_STATE, "");
}

void TestRewardsEngineClient::LoadPublisherState(
    LoadPublisherStateCallback callback) {
  std::move(callback).Run(mojom::Result::NO_PUBLISHER_STATE, "");
}

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

void TestRewardsEngineClient::GetBooleanState(
    const std::string& name,
    GetBooleanStateCallback callback) {
  std::move(callback).Run(
      state_store_.FindBoolByDottedPath(name).value_or(false));
}

void TestRewardsEngineClient::SetBooleanState(
    const std::string& name,
    bool value,
    SetBooleanStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetIntegerState(
    const std::string& name,
    GetIntegerStateCallback callback) {
  std::move(callback).Run(state_store_.FindIntByDottedPath(name).value_or(0));
}

void TestRewardsEngineClient::SetIntegerState(
    const std::string& name,
    int32_t value,
    SetIntegerStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetDoubleState(const std::string& name,
                                             GetDoubleStateCallback callback) {
  std::move(callback).Run(
      state_store_.FindDoubleByDottedPath(name).value_or(0.0));
}

void TestRewardsEngineClient::SetDoubleState(const std::string& name,
                                             double value,
                                             SetDoubleStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetStringState(const std::string& name,
                                             GetStringStateCallback callback) {
  const auto* value = state_store_.FindStringByDottedPath(name);
  std::move(callback).Run(value ? *value : base::EmptyString());
}

void TestRewardsEngineClient::SetStringState(const std::string& name,
                                             const std::string& value,
                                             SetStringStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetInt64State(const std::string& name,
                                            GetInt64StateCallback callback) {
  if (const std::string* opt = state_store_.FindStringByDottedPath(name)) {
    int64_t value;
    if (base::StringToInt64(*opt, &value)) {
      return std::move(callback).Run(value);
    }
  }

  std::move(callback).Run(0);
}

void TestRewardsEngineClient::SetInt64State(const std::string& name,
                                            int64_t value,
                                            SetInt64StateCallback callback) {
  state_store_.SetByDottedPath(name, base::NumberToString(value));
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetUint64State(const std::string& name,
                                             GetUint64StateCallback callback) {
  if (const std::string* opt = state_store_.FindStringByDottedPath(name)) {
    uint64_t value;
    if (base::StringToUint64(*opt, &value)) {
      return std::move(callback).Run(value);
    }
  }
  std::move(callback).Run(0);
}

void TestRewardsEngineClient::SetUint64State(const std::string& name,
                                             uint64_t value,
                                             SetUint64StateCallback callback) {
  state_store_.SetByDottedPath(name, base::NumberToString(value));
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetValueState(const std::string& name,
                                            GetValueStateCallback callback) {
  const auto* value = state_store_.FindByDottedPath(name);
  std::move(callback).Run(value ? value->Clone() : base::Value());
}

void TestRewardsEngineClient::SetValueState(const std::string& name,
                                            base::Value value,
                                            SetValueStateCallback callback) {
  state_store_.SetByDottedPath(name, std::move(value));
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetTimeState(const std::string& name,
                                           GetTimeStateCallback callback) {
  const auto* value = state_store_.FindByDottedPath(name);
  if (!value) {
    std::move(callback).Run(base::Time());
    return;
  }
  auto time = base::ValueToTime(*value);
  DCHECK(time);
  std::move(callback).Run(time.value_or(base::Time()));
}

void TestRewardsEngineClient::SetTimeState(const std::string& name,
                                           base::Time value,
                                           SetTimeStateCallback callback) {
  state_store_.SetByDottedPath(name, base::TimeToValue(value));
  std::move(callback).Run();
}

void TestRewardsEngineClient::ClearState(const std::string& name,
                                         ClearStateCallback callback) {
  state_store_.RemoveByDottedPath(name);
  std::move(callback).Run();
}

void TestRewardsEngineClient::GetClientCountryCode(
    GetClientCountryCodeCallback callback) {
  GetStringState(state::kDeclaredGeo, std::move(callback));
}

void TestRewardsEngineClient::GetLegacyWallet(
    GetLegacyWalletCallback callback) {
  std::move(callback).Run("");
}

void TestRewardsEngineClient::ShowNotification(
    const std::string& type,
    const std::vector<std::string>& args,
    ShowNotificationCallback callback) {}

void TestRewardsEngineClient::GetClientInfo(GetClientInfoCallback callback) {
  auto info = mojom::ClientInfo::New();
  info->platform = mojom::Platform::DESKTOP;
  info->os = mojom::OperatingSystem::UNDEFINED;
  std::move(callback).Run(std::move(info));
}

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

void TestRewardsEngineClient::ExternalWalletConnected() {}

void TestRewardsEngineClient::ExternalWalletLoggedOut() {}

void TestRewardsEngineClient::ExternalWalletReconnected() {}

void TestRewardsEngineClient::ExternalWalletDisconnected() {}

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
