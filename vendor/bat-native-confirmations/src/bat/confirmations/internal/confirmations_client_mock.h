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
      std::unique_ptr<ledger::WalletInfo>));

  MOCK_METHOD4(OnReconcileComplete, void(
      Result result,
      const std::string& viewing_id,
      ledger::REWARDS_CATEGORY category,
      const std::string& probi));

  MOCK_METHOD1(LoadLedgerState, void(
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD2(SaveLedgerState, void(
      const std::string& ledger_state,
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD1(LoadPublisherState, void(
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD2(SavePublisherState, void(
      const std::string& publisher_state,
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD2(SavePublishersList, void(
      const std::string& publisher_state,
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD1(LoadPublisherList, void(
      ledger::LedgerCallbackHandler* handler));

  MOCK_METHOD1(LoadNicewareList, void(
      ledger::GetNicewareListCallback callback));

  MOCK_METHOD2(SavePublisherInfo, void(
      std::unique_ptr<ledger::PublisherInfo> publisher_info,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(SaveActivityInfo, void(
      std::unique_ptr<ledger::PublisherInfo> publisher_info,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadPublisherInfo, void(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadActivityInfo, void(
      ledger::ActivityInfoFilter filter,
      ledger::PublisherInfoCallback callback));

  MOCK_METHOD2(LoadPanelPublisherInfo, void(
      ledger::ActivityInfoFilter filter,
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
      ledger::ActivityInfoFilter filter,
      ledger::PublisherInfoListCallback callback));

  MOCK_METHOD2(FetchGrants, void(
      const std::string& lang,
      const std::string& paymentId));

  MOCK_METHOD2(OnGrant, void(
      ledger::Result result,
      const ledger::Grant& grant));

  MOCK_METHOD2(GetGrantCaptcha, void(
      const std::string& promotion_id,
      const std::string& promotion_type));

  MOCK_METHOD2(OnGrantCaptcha, void(
      const std::string& image,
      const std::string& hint));

  MOCK_METHOD3(OnRecoverWallet, void(
      ledger::Result result,
      double balance,
      const std::vector<ledger::Grant>& grants));

  MOCK_METHOD2(OnGrantFinish, void(
      ledger::Result result,
      const ledger::Grant& grant));

  MOCK_METHOD3(OnPanelPublisherInfo, void(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo>,
      uint64_t windowId));

  MOCK_METHOD2(OnExcludedSitesChanged, void(
      const std::string& publisher_id,
      ledger::PUBLISHER_EXCLUDE exclude));

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
      const ledger::REWARDS_CATEGORY category));

  MOCK_METHOD1(GetRecurringTips, void(
      ledger::PublisherInfoListCallback callback));

  MOCK_METHOD2(OnRemoveRecurring, void(
      const std::string& publisher_key,
      ledger::RecurringRemoveCallback callback));

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

  MOCK_METHOD3(SetContributionAutoInclude, void(
      const std::string& publisher_key,
      bool excluded,
      uint64_t windowId));

  MOCK_METHOD1(SavePendingContribution, void(
      const ledger::PendingContributionList& list));

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

  MOCK_METHOD1(OnRestorePublishers, void(
      ledger::OnRestoreCallback callback));

  MOCK_METHOD1(SaveNormalizedPublisherList, void(
      const ledger::PublisherInfoListStruct& normalized_list));

  MOCK_METHOD1(SetConfirmationsIsReady, void(
      const bool is_ready));

  MOCK_METHOD0(ConfirmationsTransactionHistoryDidChange, void());

  MOCK_METHOD1(GetExcludedPublishersNumberDB, void(
      ledger::GetExcludedPublishersNumberDBCallback callback));
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_CLIENT_MOCK_H_
