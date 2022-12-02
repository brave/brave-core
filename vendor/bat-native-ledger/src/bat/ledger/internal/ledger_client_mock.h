/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_CLIENT_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_CLIENT_MOCK_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/core/test_ledger_client.h"
#include "bat/ledger/ledger_client.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace ledger {

class MockLedgerClient : public LedgerClient {
 public:
  MockLedgerClient();
  ~MockLedgerClient() override;

  absl::optional<std::string> EncryptString(const std::string& value) override;

  absl::optional<std::string> DecryptString(const std::string& value) override;

  MOCK_METHOD2(OnReconcileComplete,
               void(mojom::Result result,
                    mojom::ContributionInfoPtr contribution));

  MOCK_METHOD1(LoadLedgerState, void(
      client::OnLoadCallback callback));

  MOCK_METHOD1(LoadPublisherState, void(
      client::OnLoadCallback callback));

  MOCK_METHOD3(OnPanelPublisherInfo,
               void(mojom::Result result,
                    mojom::PublisherInfoPtr,
                    uint64_t windowId));

  MOCK_METHOD0(OnPublisherRegistryUpdated, void());

  MOCK_METHOD1(OnPublisherUpdated, void(const std::string& publisher_id));

  MOCK_METHOD3(FetchFavIcon, void(
      const std::string& url,
      const std::string& favicon_key,
      client::FetchIconCallback callback));

  MOCK_METHOD1(URIEncode, std::string(const std::string& value));

  MOCK_METHOD2(LoadURL,
               void(mojom::UrlRequestPtr request,
                    client::LoadURLCallback callback));

  MOCK_METHOD2(SetPublisherExclude, void(
      const std::string& publisher_key,
      bool exclude));

  MOCK_METHOD4(Log, void(
      const char* file,
      const int line,
      const int verbose_level,
      const std::string& message));

  MOCK_METHOD1(PublisherListNormalized,
               void(std::vector<mojom::PublisherInfoPtr> list));

  MOCK_METHOD2(SetBooleanState, void(
      const std::string& name,
      bool value));

  MOCK_CONST_METHOD1(GetBooleanState, bool(
      const std::string& name));

  MOCK_METHOD2(SetIntegerState, void(
      const std::string& name,
      int value));

  MOCK_CONST_METHOD1(GetIntegerState, int(
      const std::string& name));

  MOCK_METHOD2(SetDoubleState, void(
      const std::string& name,
      double value));

  MOCK_CONST_METHOD1(GetDoubleState, double(
      const std::string& name));

  MOCK_METHOD2(SetStringState, void(
      const std::string& name,
      const std::string& value));

  MOCK_CONST_METHOD1(GetStringState, std::string(
      const std::string& name));

  MOCK_METHOD2(SetInt64State, void(
      const std::string& name,
      int64_t value));

  MOCK_CONST_METHOD1(GetInt64State, int64_t(
      const std::string& name));

  MOCK_METHOD2(SetUint64State, void(
      const std::string& name,
      uint64_t value));

  MOCK_CONST_METHOD1(GetUint64State, uint64_t(
      const std::string& name));

  MOCK_METHOD2(SetValueState, void(const std::string& name, base::Value value));

  MOCK_CONST_METHOD1(GetValueState, base::Value(const std::string& name));

  MOCK_METHOD2(SetTimeState, void(const std::string& name, base::Time time));

  MOCK_CONST_METHOD1(GetTimeState, base::Time(const std::string& name));

  MOCK_METHOD1(ClearState, void(
      const std::string& name));

  MOCK_CONST_METHOD1(GetBooleanOption, bool(
      const std::string& name));

  MOCK_CONST_METHOD1(GetIntegerOption, int(
      const std::string& name));

  MOCK_CONST_METHOD1(GetDoubleOption, double(
      const std::string& name));

  MOCK_CONST_METHOD1(GetStringOption, std::string(
      const std::string& name));

  MOCK_CONST_METHOD1(GetInt64Option, int64_t(
      const std::string& name));

  MOCK_CONST_METHOD1(GetUint64Option, uint64_t(
      const std::string& name));

  MOCK_METHOD3(OnContributeUnverifiedPublishers,
               void(mojom::Result result,
                    const std::string& publisher_key,
                    const std::string& publisher_name));

  MOCK_METHOD3(ShowNotification,
               void(const std::string& type,
                    const std::vector<std::string>& args,
                    client::LegacyResultCallback callback));

  MOCK_METHOD0(GetClientInfo, mojom::ClientInfoPtr());

  MOCK_METHOD0(UnblindedTokensReady, void());

  MOCK_METHOD0(ReconcileStampReset, void());

  MOCK_METHOD2(RunDBTransaction,
               void(mojom::DBTransactionPtr, client::RunDBTransactionCallback));

  MOCK_METHOD1(GetCreateScript, void(client::GetCreateScriptCallback));

  MOCK_METHOD1(PendingContributionSaved, void(const mojom::Result result));

  MOCK_METHOD0(ClearAllNotifications, void());

  MOCK_CONST_METHOD0(ExternalWalletConnected, void());

  MOCK_CONST_METHOD0(ExternalWalletLoggedOut, void());

  MOCK_CONST_METHOD0(ExternalWalletReconnected, void());

  MOCK_METHOD1(DeleteLog, void(client::LegacyResultCallback callback));

  MOCK_METHOD0(GetLegacyWallet, std::string());
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_CLIENT_MOCK_H_
