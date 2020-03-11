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
using FetchIconCallback = std::function<void(bool, const std::string&)>;
using LoadURLCallback = std::function<void(const int, const std::string&,
    const std::map<std::string, std::string>& headers)>;
using OnLoadCallback = std::function<void(const Result,
                                          const std::string&)>;
using PendingContributionInfoListCallback =
    std::function<void(PendingContributionInfoList)>;
using PendingContributionsTotalCallback = std::function<void(double)>;
using GetExternalWalletsCallback =
    std::function<void(std::map<std::string, ledger::ExternalWalletPtr>)>;
using GetServerPublisherInfoCallback =
    std::function<void(ledger::ServerPublisherInfoPtr)>;
using ResultCallback = std::function<void(const Result)>;
using GetFirstContributionQueueCallback =
    std::function<void(ContributionQueuePtr)>;
using GetPromotionCallback = std::function<void(PromotionPtr)>;
using GetUnblindedTokenListCallback = std::function<void(UnblindedTokenList)>;
using GetAllPromotionsCallback = std::function<void(PromotionMap)>;

using GetTransactionReportCallback =
    std::function<void(ledger::TransactionReportInfoList)>;

using GetContributionReportCallback =
    std::function<void(ledger::ContributionReportInfoList)>;

using GetContributionInfoCallback =
    std::function<void(ContributionInfoPtr)>;

using RunDBTransactionCallback = std::function<void(DBCommandResponsePtr)>;
using GetCreateScriptCallback =
    std::function<void(const std::string&, const int)>;

using GetCredsBatchCallback = std::function<void(CredsBatchPtr)>;
using GetAllCredsBatchCallback = std::function<void(CredsBatchList)>;
using GetPromotionListCallback = std::function<void(PromotionList)>;

using SKUOrderCallback =
    std::function<void(const Result, const std::string&)>;

using TransactionCallback =
    std::function<void(const ledger::Result, const std::string&)>;

using GetSKUOrderCallback = std::function<void(SKUOrderPtr)>;

class LEDGER_EXPORT LedgerClient {
 public:
  virtual ~LedgerClient() = default;

  virtual void OnWalletProperties(
      Result result,
      ledger::WalletPropertiesPtr properties) = 0;

  virtual void OnReconcileComplete(
      const Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::RewardsType type) = 0;

  virtual void LoadLedgerState(OnLoadCallback callback) = 0;

  virtual void SaveLedgerState(
      const std::string& ledger_state,
      ResultCallback callback) = 0;

  virtual void LoadPublisherState(OnLoadCallback callback) = 0;

  virtual void SavePublisherState(
      const std::string& publisher_state,
      ResultCallback callback) = 0;

  virtual void LoadNicewareList(ledger::GetNicewareListCallback callback) = 0;

  virtual void OnPanelPublisherInfo(Result result,
                                   ledger::PublisherInfoPtr publisher_info,
                                   uint64_t windowId) = 0;

  virtual void FetchFavIcon(const std::string& url,
                            const std::string& favicon_key,
                            FetchIconCallback callback) = 0;

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

  // Logs debug information
  virtual std::unique_ptr<LogStream> Log(
      const char* file,
      int line,
      const ledger::LogLevel log_level) const = 0;

  virtual std::unique_ptr<LogStream> VerboseLog(
      const char* file,
      int line,
      int vlog_level) const = 0;

  virtual void PublisherListNormalized(ledger::PublisherInfoList list) = 0;

  virtual void SaveState(const std::string& name,
                         const std::string& value,
                         ledger::ResultCallback callback) = 0;
  virtual void LoadState(const std::string& name,
                         ledger::OnLoadCallback callback) = 0;
  virtual void ResetState(const std::string& name,
                          ledger::ResultCallback callback) = 0;

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
      ledger::ResultCallback callback) = 0;

  virtual void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee) = 0;

  virtual ledger::TransferFeeList GetTransferFees(
      const std::string& wallet_type) = 0;

  virtual void RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) = 0;

  virtual ledger::ClientInfoPtr GetClientInfo() = 0;

  virtual void UnblindedTokensReady() = 0;

  virtual void ReconcileStampReset() = 0;

  virtual void RunDBTransaction(
      ledger::DBTransactionPtr transaction,
      ledger::RunDBTransactionCallback callback) = 0;

  virtual void GetCreateScript(ledger::GetCreateScriptCallback callback) = 0;

  virtual void PendingContributionSaved(const ledger::Result result) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CLIENT_H_
