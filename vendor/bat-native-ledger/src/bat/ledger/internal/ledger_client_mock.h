/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_CLIENT_MOCK_H_
#define BAT_LEDGER_LEDGER_CLIENT_MOCK_H_

#include <stdint.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger_client.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace ledger {

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

class MockLedgerClient : public LedgerClient {
 public:
  MockLedgerClient();
  ~MockLedgerClient() override;

  MOCK_CONST_METHOD0(GenerateGUID, std::string());

  MOCK_METHOD1(OnWalletInitialized, void(
      ledger::Result result));

  MOCK_METHOD0(FetchWalletProperties, void());

  MOCK_METHOD2(OnWalletProperties, void(
      ledger::Result result,
      ledger::WalletPropertiesPtr));

  MOCK_METHOD4(OnReconcileComplete, void(
      Result result,
      const std::string& viewing_id,
      const double amount,
      const ledger::RewardsType type));

  MOCK_METHOD1(LoadLedgerState, void(
      ledger::OnLoadCallback callback));

  MOCK_METHOD2(SaveLedgerState, void(
      const std::string& ledger_state,
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD1(LoadPublisherState, void(
      ledger::OnLoadCallback callback));

  MOCK_METHOD2(SavePublisherState, void(
      const std::string& publisher_state,
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD1(LoadNicewareList, void(
      ledger::GetNicewareListCallback callback));

  MOCK_METHOD2(SavePublisherInfo, void(
      ledger::PublisherInfoPtr publisher_info,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadPublisherInfo, void(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadPanelPublisherInfo, void(
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadMediaPublisherInfo, void(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(SaveMediaPublisherInfo, void(
      const std::string& media_key,
      const std::string& publisher_id));

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

  MOCK_METHOD2(SaveContributionInfo, void(
      ledger::ContributionInfoPtr info,
      ledger::ResultCallback callback));

  MOCK_METHOD2(SaveRecurringTip, void(
      ledger::RecurringTipPtr info,
      ledger::SaveRecurringTipCallback callback));

  MOCK_METHOD1(GetRecurringTips, void(
      ledger::PublisherInfoListCallback callback));

  MOCK_METHOD1(GetOneTimeTips, void(
      ledger::PublisherInfoListCallback callback));

  MOCK_METHOD2(RemoveRecurringTip, void(
      const std::string& publisher_key,
      ledger::RemoveRecurringTipCallback callback));

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
      const ledger::UrlMethod method,
      ledger::LoadURLCallback callback));

  MOCK_METHOD2(SetPublisherExclude, void(
      const std::string& publisher_key,
      bool exclude));

  MOCK_METHOD2(SavePendingContribution, void(
      ledger::PendingContributionList list,
      ledger::SavePendingContributionCallback callback));

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
      ledger::OnSaveCallback callback));

  MOCK_METHOD2(LoadState, void(
      const std::string& name,
      ledger::OnLoadCallback callback));

  MOCK_METHOD2(ResetState, void(
      const std::string& name,
      ledger::OnResetCallback callback));

  MOCK_METHOD1(RestorePublishers, void(
      ledger::RestorePublishersCallback callback));

  MOCK_METHOD1(PublisherListNormalized, void(ledger::PublisherInfoList list));

  MOCK_METHOD1(SetConfirmationsIsReady, void(
      const bool is_ready));

  MOCK_METHOD0(ConfirmationsTransactionHistoryDidChange, void());

  MOCK_METHOD1(GetPendingContributions, void(
      ledger::PendingContributionInfoListCallback callback));

  MOCK_METHOD2(RemovePendingContribution, void(
      const uint64_t id,
      ledger::RemovePendingContributionCallback callback));

  MOCK_METHOD1(RemoveAllPendingContributions, void(
    ledger::RemovePendingContributionCallback callback));

  MOCK_METHOD1(GetPendingContributionsTotal, void(
    ledger::PendingContributionsTotalCallback callback));

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
      ledger::ShowNotificationCallback callback));

  MOCK_METHOD2(ClearAndInsertServerPublisherList, void(
      ledger::ServerPublisherInfoList list,
      ledger::ClearAndInsertServerPublisherListCallback callback));

  MOCK_METHOD2(GetServerPublisherInfo, void(
      const std::string& publisher_key,
      ledger::GetServerPublisherInfoCallback callback));

  MOCK_METHOD2(SetTransferFee, void(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee));

  MOCK_METHOD1(GetTransferFees, ledger::TransferFeeList(
      const std::string& wallet_type));

  MOCK_METHOD2(RemoveTransferFee, void(
    const std::string& wallet_type,
    const std::string& id));

  MOCK_METHOD2(InsertOrUpdateContributionQueue, void(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback));

  MOCK_METHOD2(DeleteContributionQueue, void(
    const uint64_t id,
    ledger::ResultCallback callback));

  MOCK_METHOD1(GetFirstContributionQueue, void(
    ledger::GetFirstContributionQueueCallback callback));

  MOCK_METHOD2(InsertOrUpdatePromotion, void(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback));

  MOCK_METHOD2(GetPromotion, void(
    const std::string& id,
    ledger::GetPromotionCallback callback));

  MOCK_METHOD1(GetAllPromotions, void(
    ledger::GetAllPromotionsCallback callback));

  MOCK_METHOD2(DeletePromotionList, void(
      const std::vector<std::string>& id_list,
      ledger::ResultCallback callback));

  MOCK_METHOD2(SaveUnblindedTokenList, void(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback));

  MOCK_METHOD1(GetAllUnblindedTokens, void(
    ledger::GetAllUnblindedTokensCallback callback));

  MOCK_METHOD2(DeleteUnblindedTokens, void(
    const std::vector<std::string>& id_list,
    ledger::ResultCallback callback));

  MOCK_METHOD0(GetClientInfo, ledger::ClientInfoPtr());

  MOCK_METHOD0(UnblindedTokensReady, void());

  MOCK_METHOD2(DeleteUnblindedTokensForPromotion,
      void(const std::string& promotion_id, ledger::ResultCallback));

  MOCK_METHOD3(GetTransactionReport, void(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback));

  MOCK_METHOD3(GetContributionReport, void(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback));

  MOCK_METHOD1(GetIncompleteContributions, void(
      ledger::GetIncompleteContributionsCallback callback));

  MOCK_METHOD2(GetContributionInfo, void(
      const std::string& contribution_id,
      GetContributionInfoCallback callback));

  MOCK_METHOD4(UpdateContributionInfoStepAndCount, void(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count,
      ResultCallback callback));

  MOCK_METHOD3(UpdateContributionInfoContributedAmount, void(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ResultCallback callback));

  MOCK_METHOD0(ReconcileStampReset, void());

  MOCK_METHOD2(RunDBTransaction, void(
      ledger::DBTransactionPtr,
      ledger::RunDBTransactionCallback));

  MOCK_METHOD1(GetCreateScript, void(ledger::GetCreateScriptCallback));
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CLIENT_MOCK_H_
