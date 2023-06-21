/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/test/test_ledger_client.h"

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
#include "brave/components/brave_rewards/common/mojom/ledger.mojom.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

std::string FakeEncryption::EncryptString(const std::string& value) {
  return "ENCRYPTED:" + value;
}

absl::optional<std::string> FakeEncryption::DecryptString(
    const std::string& value) {
  size_t pos = value.find("ENCRYPTED:");
  if (pos == 0) {
    return value.substr(10);
  }

  return {};
}

std::string FakeEncryption::Base64EncryptString(const std::string& value) {
  std::string fake_encrypted = EncryptString(value);
  std::string encoded;
  base::Base64Encode(fake_encrypted, &encoded);
  return encoded;
}

absl::optional<std::string> FakeEncryption::Base64DecryptString(
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
  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("brave");
  path = path.AppendASCII("components");
  path = path.AppendASCII("brave_rewards");
  path = path.AppendASCII("core");
  path = path.AppendASCII("test");
  path = path.AppendASCII("data");
  return path;
}

TestLedgerClient::TestLedgerClient() : ledger_database_(base::FilePath()) {
  CHECK(ledger_database_.GetInternalDatabaseForTesting()->OpenInMemory());
}

TestLedgerClient::~TestLedgerClient() = default;

void TestLedgerClient::LoadLedgerState(LoadLedgerStateCallback callback) {
  std::move(callback).Run(mojom::Result::NO_LEDGER_STATE, "");
}

void TestLedgerClient::LoadPublisherState(LoadPublisherStateCallback callback) {
  std::move(callback).Run(mojom::Result::NO_PUBLISHER_STATE, "");
}

void TestLedgerClient::OnReconcileComplete(
    mojom::Result result,
    mojom::ContributionInfoPtr contribution) {}

void TestLedgerClient::OnPanelPublisherInfo(
    mojom::Result result,
    mojom::PublisherInfoPtr publisher_info,
    uint64_t window_id) {}

void TestLedgerClient::FetchFavIcon(const std::string& url,
                                    const std::string& favicon_key,
                                    FetchFavIconCallback callback) {
  std::move(callback).Run(true, favicon_key);
}

void TestLedgerClient::LoadURL(mojom::UrlRequestPtr request,
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

void TestLedgerClient::PublisherListNormalized(
    std::vector<mojom::PublisherInfoPtr> list) {}

void TestLedgerClient::OnPublisherRegistryUpdated() {}

void TestLedgerClient::OnPublisherUpdated(const std::string& publisher_id) {}

void TestLedgerClient::GetBooleanState(const std::string& name,
                                       GetBooleanStateCallback callback) {
  std::move(callback).Run(
      state_store_.FindBoolByDottedPath(name).value_or(false));
}

void TestLedgerClient::SetBooleanState(const std::string& name,
                                       bool value,
                                       SetBooleanStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestLedgerClient::GetIntegerState(const std::string& name,
                                       GetIntegerStateCallback callback) {
  std::move(callback).Run(state_store_.FindIntByDottedPath(name).value_or(0));
}

void TestLedgerClient::SetIntegerState(const std::string& name,
                                       int32_t value,
                                       SetIntegerStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestLedgerClient::GetDoubleState(const std::string& name,
                                      GetDoubleStateCallback callback) {
  std::move(callback).Run(
      state_store_.FindDoubleByDottedPath(name).value_or(0.0));
}

void TestLedgerClient::SetDoubleState(const std::string& name,
                                      double value,
                                      SetDoubleStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestLedgerClient::GetStringState(const std::string& name,
                                      GetStringStateCallback callback) {
  const auto* value = state_store_.FindStringByDottedPath(name);
  std::move(callback).Run(value ? *value : base::EmptyString());
}

void TestLedgerClient::SetStringState(const std::string& name,
                                      const std::string& value,
                                      SetStringStateCallback callback) {
  state_store_.SetByDottedPath(name, value);
  std::move(callback).Run();
}

void TestLedgerClient::GetInt64State(const std::string& name,
                                     GetInt64StateCallback callback) {
  if (const std::string* opt = state_store_.FindStringByDottedPath(name)) {
    int64_t value;
    if (base::StringToInt64(*opt, &value)) {
      return std::move(callback).Run(value);
    }
  }

  std::move(callback).Run(0);
}

void TestLedgerClient::SetInt64State(const std::string& name,
                                     int64_t value,
                                     SetInt64StateCallback callback) {
  state_store_.SetByDottedPath(name, base::NumberToString(value));
  std::move(callback).Run();
}

void TestLedgerClient::GetUint64State(const std::string& name,
                                      GetUint64StateCallback callback) {
  if (const std::string* opt = state_store_.FindStringByDottedPath(name)) {
    uint64_t value;
    if (base::StringToUint64(*opt, &value)) {
      return std::move(callback).Run(value);
    }
  }
  std::move(callback).Run(0);
}

void TestLedgerClient::SetUint64State(const std::string& name,
                                      uint64_t value,
                                      SetUint64StateCallback callback) {
  state_store_.SetByDottedPath(name, base::NumberToString(value));
  std::move(callback).Run();
}

void TestLedgerClient::GetValueState(const std::string& name,
                                     GetValueStateCallback callback) {
  const auto* value = state_store_.FindByDottedPath(name);
  std::move(callback).Run(value ? value->Clone() : base::Value());
}

void TestLedgerClient::SetValueState(const std::string& name,
                                     base::Value value,
                                     SetValueStateCallback callback) {
  state_store_.SetByDottedPath(name, std::move(value));
  std::move(callback).Run();
}

void TestLedgerClient::GetTimeState(const std::string& name,
                                    GetTimeStateCallback callback) {
  const auto* value = state_store_.FindByDottedPath(name);
  DCHECK(value);
  if (!value) {
    return std::move(callback).Run(base::Time());
  }

  auto time = base::ValueToTime(*value);
  DCHECK(time);
  std::move(callback).Run(time.value_or(base::Time()));
}

void TestLedgerClient::SetTimeState(const std::string& name,
                                    base::Time value,
                                    SetTimeStateCallback callback) {
  state_store_.SetByDottedPath(name, base::TimeToValue(value));
  std::move(callback).Run();
}

void TestLedgerClient::ClearState(const std::string& name,
                                  ClearStateCallback callback) {
  state_store_.RemoveByDottedPath(name);
  std::move(callback).Run();
}

void TestLedgerClient::IsBitFlyerRegion(IsBitFlyerRegionCallback callback) {
  std::move(callback).Run(is_bitflyer_region_);
}

void TestLedgerClient::GetLegacyWallet(GetLegacyWalletCallback callback) {
  std::move(callback).Run("");
}

void TestLedgerClient::ShowNotification(const std::string& type,
                                        const std::vector<std::string>& args,
                                        ShowNotificationCallback callback) {}

void TestLedgerClient::GetClientInfo(GetClientInfoCallback callback) {
  auto info = mojom::ClientInfo::New();
  info->platform = mojom::Platform::DESKTOP;
  info->os = mojom::OperatingSystem::UNDEFINED;
  std::move(callback).Run(std::move(info));
}

void TestLedgerClient::UnblindedTokensReady() {}

void TestLedgerClient::ReconcileStampReset() {}

void TestLedgerClient::RunDBTransaction(mojom::DBTransactionPtr transaction,
                                        RunDBTransactionCallback callback) {
  auto response = ledger_database_.RunTransaction(std::move(transaction));
  std::move(callback).Run(std::move(response));
}

void TestLedgerClient::Log(const std::string& file,
                           int32_t line,
                           int32_t verbose_level,
                           const std::string& message) {
  int vlog_level =
      logging::GetVlogLevelHelper(file.c_str(), strlen(file.c_str()));
  if (verbose_level <= vlog_level) {
    logging::LogMessage(file.c_str(), line, -verbose_level).stream() << message;
  }

  if (log_callback_) {
    log_callback_.Run(message);
  }
}

void TestLedgerClient::ClearAllNotifications() {}

void TestLedgerClient::ExternalWalletConnected() {}

void TestLedgerClient::ExternalWalletLoggedOut() {}

void TestLedgerClient::ExternalWalletReconnected() {}

void TestLedgerClient::ExternalWalletDisconnected() {}

void TestLedgerClient::DeleteLog(DeleteLogCallback callback) {
  std::move(callback).Run(mojom::Result::LEDGER_OK);
}

void TestLedgerClient::EncryptString(const std::string& value,
                                     EncryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::EncryptString(value));
}

void TestLedgerClient::DecryptString(const std::string& value,
                                     DecryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::DecryptString(value));
}

void TestLedgerClient::SetIsBitFlyerRegionForTesting(bool is_bitflyer_region) {
  is_bitflyer_region_ = is_bitflyer_region;
}

void TestLedgerClient::AddNetworkResultForTesting(
    const std::string& url,
    mojom::UrlMethod method,
    mojom::UrlResponsePtr response) {
  network_results_.emplace_back(url, method, std::move(response));
}

void TestLedgerClient::SetLogCallbackForTesting(LogCallback callback) {
  log_callback_ = std::move(callback);
}

}  // namespace brave_rewards::internal
