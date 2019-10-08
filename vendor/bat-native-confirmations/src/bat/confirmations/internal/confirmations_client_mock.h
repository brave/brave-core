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

class MockConfirmationsClient : public ConfirmationsClient {
 public:
  MockConfirmationsClient();
  ~MockConfirmationsClient() override;

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
      const std::string& probi,
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

  MOCK_METHOD2(SaveActivityInfo, void(
      ledger::PublisherInfoPtr publisher_info,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadPublisherInfo, void(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadActivityInfo, void(
      ledger::ActivityInfoFilterPtr filter,
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

  MOCK_METHOD4(GetActivityInfoList, void(
      uint32_t start,
      uint32_t limit,
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoListCallback callback));

  MOCK_METHOD2(FetchGrants, void(
      const std::string& lang,
      const std::string& paymentId));

  MOCK_METHOD2(OnGrant, void(
      ledger::Result result,
      ledger::GrantPtr grant));

  MOCK_METHOD2(GetGrantCaptcha, void(
      const std::string& promotion_id,
      const std::string& promotion_type));

  MOCK_METHOD2(OnGrantCaptcha, void(
      const std::string& image,
      const std::string& hint));

  MOCK_METHOD3(OnRecoverWallet, void(
      ledger::Result result,
      double balance,
      std::vector<ledger::GrantPtr> grants));

  MOCK_METHOD2(OnGrantFinish, void(
      ledger::Result result,
      ledger::GrantPtr grant));

  MOCK_METHOD3(OnPanelPublisherInfo, void(
      ledger::Result result,
      ledger::PublisherInfoPtr,
      uint64_t windowId));

  MOCK_METHOD3(FetchFavIcon, void(
      const std::string& url,
      const std::string& favicon_key,
      ledger::FetchIconCallback callback));

  MOCK_METHOD6(SaveContributionInfo, void(
      const std::string& probi,
      const int month,
      const int year,
      const uint32_t date,
      const std::string& publisher_key,
      const ledger::RewardsType type));

  MOCK_METHOD2(SaveRecurringTip, void(
      ledger::ContributionInfoPtr info,
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
      const URLRequestMethod method,
      URLRequestCallback callback));

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

  MOCK_METHOD1(SaveNormalizedPublisherList, void(
      ledger::PublisherInfoList normalized_list));

  MOCK_METHOD1(SetConfirmationsIsReady, void(
      const bool is_ready));

  MOCK_METHOD0(ConfirmationsTransactionHistoryDidChange, void());

  MOCK_METHOD1(GetPendingContributions, void(
      ledger::PendingContributionInfoListCallback callback));

  MOCK_METHOD4(RemovePendingContribution, void(
      const std::string& publisher_key,
      const std::string& viewing_id,
      uint64_t added_date,
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

  MOCK_METHOD1(GetExternalWallets, void(
      ledger::GetExternalWalletsCallback callback));

  MOCK_METHOD2(SaveExternalWallet, void(
      const std::string& wallet_type,
      ledger::ExternalWalletPtr wallet));

  MOCK_METHOD3(ShowNotification, void(
      const std::string& type,
      const std::vector<std::string>& args,
      ledger::ShowNotificationCallback callback));

  MOCK_METHOD2(DeleteActivityInfo, void(
      const std::string& publisher_key,
      ledger::DeleteActivityInfoCallback callback));

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
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_CLIENT_MOCK_H_
