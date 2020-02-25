/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_CLIENT_H_
#define BAT_LEDGER_LEDGER_CLIENT_H_

#include <functional>
#include <memory>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <map>

#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/export.h"
#include "bat/ledger/ledger_callback_handler.h"

namespace confirmations {
class LogStream;
}

namespace ledger {

LEDGER_EXPORT enum LogLevel {
  LOG_ERROR = 1,
  LOG_WARNING = 2,
  LOG_INFO = 3,
  LOG_DEBUG = 4,
  LOG_REQUEST = 5,
  LOG_RESPONSE = 6
};

class LEDGER_EXPORT LogStream {
 public:
  virtual ~LogStream() = default;
  virtual std::ostream& stream() = 0;
};

using PublisherInfoCallback =
    std::function<void(const Result, PublisherInfoPtr)>;
// TODO(nejczdovc) we should be providing result back as well
using PublisherInfoListCallback =
    std::function<void(PublisherInfoList)>;
using GetNicewareListCallback =
    std::function<void(const Result, const std::string&)>;
using RemoveRecurringTipCallback = std::function<void(const Result)>;
using FetchIconCallback = std::function<void(bool, const std::string&)>;
using LoadURLCallback = std::function<void(const int, const std::string&,
    const std::map<std::string, std::string>& headers)>;
using RestorePublishersCallback = std::function<void(const Result)>;
using OnSaveCallback = std::function<void(const Result)>;
using OnLoadCallback = std::function<void(const Result,
                                          const std::string&)>;
using OnResetCallback = std::function<void(const Result)>;
using PendingContributionInfoListCallback =
    std::function<void(PendingContributionInfoList)>;
using RemovePendingContributionCallback = std::function<void(const Result)>;
using PendingContributionsTotalCallback = std::function<void(double)>;
using GetExternalWalletsCallback =
    std::function<void(std::map<std::string, ledger::ExternalWalletPtr>)>;
using ShowNotificationCallback = std::function<void(const Result)>;
using SavePendingContributionCallback = std::function<void(const Result)>;
using SaveRecurringTipCallback = std::function<void(const Result)>;
using ClearAndInsertServerPublisherListCallback =
    std::function<void(const Result)>;
using GetServerPublisherInfoCallback =
    std::function<void(ledger::ServerPublisherInfoPtr)>;
using ResultCallback = std::function<void(const Result)>;
using GetFirstContributionQueueCallback =
    std::function<void(ContributionQueuePtr)>;
using GetPromotionCallback = std::function<void(PromotionPtr)>;
using GetAllUnblindedTokensCallback = std::function<void(UnblindedTokenList)>;
using GetAllPromotionsCallback = std::function<void(PromotionMap)>;

using GetTransactionReportCallback =
    std::function<void(ledger::TransactionReportInfoList)>;

using GetContributionReportCallback =
    std::function<void(ledger::ContributionReportInfoList)>;

using GetIncompleteContributionsCallback =
    std::function<void(ContributionInfoList)>;

using GetContributionInfoCallback =
    std::function<void(ContributionInfoPtr)>;

using RunDBTransactionCallback = std::function<void(DBCommandResponsePtr)>;
using GetCreateScriptCallback =
    std::function<void(const std::string&, const int)>;

class LEDGER_EXPORT LedgerClient {
 public:
  virtual ~LedgerClient() = default;

  // called when the wallet creation has completed
  virtual std::string GenerateGUID() const = 0;

  virtual void OnWalletProperties(
      Result result,
      ledger::WalletPropertiesPtr properties) = 0;

  virtual void OnReconcileComplete(
      const Result result,
      const std::string& viewing_id,
      const double amount,
      const ledger::RewardsType type) = 0;

  virtual void LoadLedgerState(OnLoadCallback callback) = 0;

  virtual void SaveLedgerState(const std::string& ledger_state,
                               LedgerCallbackHandler* handler) = 0;

  virtual void LoadPublisherState(OnLoadCallback callback) = 0;

  virtual void SavePublisherState(const std::string& publisher_state,
                                  LedgerCallbackHandler* handler) = 0;

  virtual void LoadNicewareList(ledger::GetNicewareListCallback callback) = 0;

  virtual void SavePublisherInfo(PublisherInfoPtr publisher_info,
                                 PublisherInfoCallback callback) = 0;

  virtual void LoadPublisherInfo(const std::string& publisher_key,
                                 PublisherInfoCallback callback) = 0;

  virtual void LoadPanelPublisherInfo(ActivityInfoFilterPtr filter,
                                      PublisherInfoCallback callback) = 0;

  virtual void LoadMediaPublisherInfo(const std::string& media_key,
                                PublisherInfoCallback callback) = 0;

  virtual void SaveMediaPublisherInfo(const std::string& media_key,
                                const std::string& publisher_id) = 0;

  virtual void OnPanelPublisherInfo(Result result,
                                   ledger::PublisherInfoPtr publisher_info,
                                   uint64_t windowId) = 0;

  virtual void FetchFavIcon(const std::string& url,
                            const std::string& favicon_key,
                            FetchIconCallback callback) = 0;

  virtual void SaveContributionInfo(
      ledger::ContributionInfoPtr info,
      ledger::ResultCallback callback) = 0;

  virtual void SaveRecurringTip(
      ledger::RecurringTipPtr info,
      ledger::SaveRecurringTipCallback callback) = 0;

  virtual void GetRecurringTips(
      ledger::PublisherInfoListCallback callback) = 0;

  virtual void GetOneTimeTips(
      ledger::PublisherInfoListCallback callback) = 0;

  virtual void RemoveRecurringTip(
      const std::string& publisher_key,
      ledger::RemoveRecurringTipCallback callback) = 0;

  // uint64_t time_offset (input): timer offset in seconds.
  // uint32_t timer_id (output) : 0 in case of failure
  virtual void SetTimer(uint64_t time_offset, uint32_t* timer_id) = 0;
  virtual void KillTimer(const uint32_t timer_id) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual void LoadURL(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::UrlMethod method,
      ledger::LoadURLCallback callback) = 0;

  virtual void SavePendingContribution(
      ledger::PendingContributionList list,
      ledger::SavePendingContributionCallback callback) = 0;

  // Logs debug information
  virtual std::unique_ptr<LogStream> Log(
      const char* file,
      int line,
      const ledger::LogLevel log_level) const = 0;

  virtual std::unique_ptr<LogStream> VerboseLog(
      const char* file,
      int line,
      int vlog_level) const = 0;

  virtual void RestorePublishers(
    ledger::RestorePublishersCallback callback) = 0;

  virtual void PublisherListNormalized(ledger::PublisherInfoList list) = 0;

  virtual void SaveState(const std::string& name,
                         const std::string& value,
                         ledger::OnSaveCallback callback) = 0;
  virtual void LoadState(const std::string& name,
                         ledger::OnLoadCallback callback) = 0;
  virtual void ResetState(const std::string& name,
                          ledger::OnResetCallback callback) = 0;

  virtual void SetBooleanState(const std::string& name, bool value) = 0;
  virtual bool GetBooleanState(const std::string& name) const = 0;
  virtual void SetIntegerState(const std::string& name, int value) = 0;
  virtual int GetIntegerState(const std::string& name) const = 0;
  virtual void SetDoubleState(const std::string& name, double value) = 0;
  virtual double GetDoubleState(const std::string& name) const = 0;
  virtual void SetStringState(const std::string& name,
                              const std::string& value) = 0;
  virtual std::string GetStringState(const std::string& name) const = 0;
  virtual void SetInt64State(const std::string& name, int64_t value) = 0;
  virtual int64_t GetInt64State(const std::string& name) const = 0;
  virtual void SetUint64State(const std::string& name, uint64_t value) = 0;
  virtual uint64_t GetUint64State(const std::string& name) const = 0;
  virtual void ClearState(const std::string& name) = 0;

  // Use option getter to get client specific static value
  virtual bool GetBooleanOption(const std::string& name) const = 0;
  virtual int GetIntegerOption(const std::string& name) const = 0;
  virtual double GetDoubleOption(const std::string& name) const = 0;
  virtual std::string GetStringOption(const std::string& name) const = 0;
  virtual int64_t GetInt64Option(const std::string& name) const = 0;
  virtual uint64_t GetUint64Option(const std::string& name) const = 0;

  virtual void SetConfirmationsIsReady(const bool is_ready) = 0;

  virtual void ConfirmationsTransactionHistoryDidChange() = 0;

  virtual void GetPendingContributions(
      ledger::PendingContributionInfoListCallback callback) = 0;

  virtual void RemovePendingContribution(
      const uint64_t id,
      ledger::RemovePendingContributionCallback callback) = 0;

  virtual void RemoveAllPendingContributions(
      ledger::RemovePendingContributionCallback callback) = 0;

  virtual void GetPendingContributionsTotal(
      ledger::PendingContributionsTotalCallback callback) = 0;

  virtual void OnContributeUnverifiedPublishers(
      Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) = 0;

  virtual void GetExternalWallets(
      GetExternalWalletsCallback callback) = 0;

  virtual void SaveExternalWallet(const std::string& wallet_type,
                                  ledger::ExternalWalletPtr wallet) = 0;

  virtual void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ledger::ShowNotificationCallback callback) = 0;

  virtual void ClearAndInsertServerPublisherList(
      ledger::ServerPublisherInfoList list,
      ledger::ClearAndInsertServerPublisherListCallback callback) = 0;

  virtual void GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) = 0;

  virtual void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee) = 0;

  virtual ledger::TransferFeeList GetTransferFees(
      const std::string& wallet_type) = 0;

  virtual void RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) = 0;

  virtual void InsertOrUpdateContributionQueue(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback) = 0;

  virtual void DeleteContributionQueue(
    const uint64_t id,
    ledger::ResultCallback callback) = 0;

  virtual void GetFirstContributionQueue(
    ledger::GetFirstContributionQueueCallback callback) = 0;

  virtual void InsertOrUpdatePromotion(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback) = 0;

  virtual void GetPromotion(
    const std::string& id,
    ledger::GetPromotionCallback callback) = 0;

  virtual void GetAllPromotions(
    ledger::GetAllPromotionsCallback callback) = 0;

  virtual void DeletePromotionList(
      const std::vector<std::string>& id_list,
      ledger::ResultCallback callback) = 0;

  virtual void SaveUnblindedTokenList(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) = 0;

  virtual void GetAllUnblindedTokens(
    ledger::GetAllUnblindedTokensCallback callback) = 0;

  virtual void DeleteUnblindedTokens(
      const std::vector<std::string>& id_list,
      ledger::ResultCallback callback) = 0;

  virtual void DeleteUnblindedTokensForPromotion(
      const std::string& promotion_id,
      ledger::ResultCallback callback) = 0;

  virtual ledger::ClientInfoPtr GetClientInfo() = 0;

  virtual void UnblindedTokensReady() = 0;

  virtual void GetTransactionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback) = 0;

  virtual void GetContributionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback) = 0;

  virtual void GetIncompleteContributions(
      ledger::GetIncompleteContributionsCallback callback) = 0;

  virtual void GetContributionInfo(
      const std::string& contribution_id,
      GetContributionInfoCallback callback) = 0;

  virtual void UpdateContributionInfoStepAndCount(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count,
      ResultCallback callback) = 0;

  virtual void UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ResultCallback callback) = 0;

  virtual void ReconcileStampReset() = 0;

  virtual void RunDBTransaction(
      ledger::DBTransactionPtr transaction,
      ledger::RunDBTransactionCallback callback) = 0;

  virtual void GetCreateScript(ledger::GetCreateScriptCallback callback) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CLIENT_H_
