/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_CLIENT_MOCK_H_
#define BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_CLIENT_MOCK_H_

#include <stdint.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/confirmations.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace confirmations {

class MockLogStreamImpl : public ledger::LogStream {
 public:
  MockLogStreamImpl(
      const char* file,
      int line,
      const ledger::LogLevel log_level);
  std::ostream& stream() override;

 private:
  // Not copyable, not assignable
  MockLogStreamImpl(const MockLogStreamImpl&) = delete;
  MockLogStreamImpl& operator=(const MockLogStreamImpl&) = delete;
};

class MockVerboseLogStreamImpl : public ledger::LogStream {
 public:
  MockVerboseLogStreamImpl(
      const char* file,
      int line,
      int vlog_level);
  std::ostream& stream() override;

 private:
  // Not copyable, not assignable
  MockVerboseLogStreamImpl(const MockVerboseLogStreamImpl&) = delete;
  MockVerboseLogStreamImpl& operator=(const MockVerboseLogStreamImpl&) = delete;
};

class ConfirmationsClientMock : public ConfirmationsClient {
 public:
  ConfirmationsClientMock();
  ~ConfirmationsClientMock() override;

  MOCK_METHOD1(OnWalletInitialized, void(
      ledger::Result result));

  MOCK_METHOD0(FetchWalletProperties, void());

  MOCK_METHOD2(OnWalletProperties, void(
      ledger::Result result,
      ledger::WalletPropertiesPtr));

  MOCK_METHOD4(OnReconcileComplete, void(
      Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::RewardsType type));

  MOCK_METHOD1(LoadLedgerState, void(
      ledger::OnLoadCallback callback));

  MOCK_METHOD2(SaveLedgerState, void(
      const std::string& ledger_state,
      ledger::ResultCallback callback));

  MOCK_METHOD1(LoadPublisherState, void(
      ledger::OnLoadCallback callback));

  MOCK_METHOD2(SavePublisherState, void(
      const std::string& publisher_state,
      ledger::ResultCallback callback));

  MOCK_METHOD1(LoadNicewareList, void(
      ledger::GetNicewareListCallback callback));

  MOCK_METHOD2(SaveActivityInfo, void(
      ledger::PublisherInfoPtr publisher_info,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadActivityInfo, void(
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD0(FetchPromotions, void());

  MOCK_METHOD1(ClaimPromotion, void(const std::string& promotion_id));

  MOCK_METHOD2(OnRecoverWallet, void(
      ledger::Result result,
      double balance));

  MOCK_METHOD3(OnPanelPublisherInfo, void(
      ledger::Result result,
      ledger::PublisherInfoPtr,
      uint64_t windowId));

  MOCK_METHOD3(FetchFavIcon, void(
      const std::string& url,
      const std::string& favicon_key,
      ledger::FetchIconCallback callback));

  MOCK_METHOD2(SetTimer, void(
      uint64_t time_offset,
      uint32_t* timer_id));

  MOCK_METHOD1(KillTimer, void(
      const uint32_t timer_id));

  MOCK_METHOD1(URIEncode, std::string(
      const std::string& value));

  MOCK_METHOD6(LoadURL, void(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback));

  MOCK_METHOD2(SetPublisherExclude, void(
      const std::string& publisher_key,
      bool exclude));

  std::unique_ptr<ledger::LogStream> Log(
      const char* file,
      const int line,
      const ledger::LogLevel log_level) const;

  std::unique_ptr<ledger::LogStream> VerboseLog(
      const char* file,
      int line,
      int vlog_level) const;

  MOCK_METHOD3(SaveState, void(
      const std::string& name,
      const std::string& value,
      ledger::ResultCallback callback));

  MOCK_METHOD2(LoadState, void(
      const std::string& name,
      ledger::OnLoadCallback callback));

  MOCK_METHOD2(ResetState, void(
      const std::string& name,
      ledger::ResultCallback callback));

  MOCK_METHOD1(PublisherListNormalized, void(
      ledger::PublisherInfoList list));

  MOCK_METHOD1(SetConfirmationsIsReady, void(
      const bool is_ready));

  MOCK_METHOD0(ConfirmationsTransactionHistoryDidChange, void());

  MOCK_METHOD3(OnContributeUnverifiedPublishers, void(
      ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name));

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

  MOCK_METHOD1(GetExternalWallets, void(
      ledger::GetExternalWalletsCallback callback));

  MOCK_METHOD2(SaveExternalWallet, void(
      const std::string& wallet_type,
      ledger::ExternalWalletPtr wallet));

  MOCK_METHOD3(ShowNotification, void(
      const std::string& type,
      const std::vector<std::string>& args,
      ledger::ResultCallback callback));

  MOCK_METHOD2(SetTransferFee, void(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee));

  MOCK_METHOD1(GetTransferFees, ledger::TransferFeeList(
      const std::string& wallet_type));

  MOCK_METHOD2(RemoveTransferFee, void(
    const std::string& wallet_type,
    const std::string& id));

  MOCK_METHOD0(GetClientInfo, ledger::ClientInfoPtr());

  MOCK_METHOD0(UnblindedTokensReady, void());

  MOCK_METHOD0(ReconcileStampReset, void());

  MOCK_METHOD2(RunDBTransaction, void(
      ledger::DBTransactionPtr transaction,
      ledger::RunDBTransactionCallback callback));

  MOCK_METHOD1(GetCreateScript, void(ledger::GetCreateScriptCallback callback));

  MOCK_METHOD1(PendingContributionSaved, void(const ledger::Result result));
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_CLIENT_MOCK_H_
