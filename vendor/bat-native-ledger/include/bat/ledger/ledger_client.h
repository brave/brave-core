/* This Source Code Form is subject to the terms of the Mozilla Public
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

#include "bat/ledger/balance_report_info.h"
#include "bat/ledger/export.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/media_publisher_info.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/wallet_info.h"
#include "bat/ledger/grant.h"
#include "bat/ledger/pending_contribution.h"

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

LEDGER_EXPORT enum URL_METHOD {
  GET = 0,
  PUT = 1,
  POST = 2
};

class LEDGER_EXPORT LogStream {
 public:
  virtual ~LogStream() = default;
  virtual std::ostream& stream() = 0;
};

using PublisherInfoCallback = std::function<void(Result,
    std::unique_ptr<PublisherInfo>)>;
// TODO(nejczdovc) we should be providing result back as well
using PublisherInfoListCallback =
    std::function<void(const PublisherInfoList&, uint32_t /* next_record */)>;
using GetNicewareListCallback =
    std::function<void(Result, const std::string&)>;
using RecurringDonationCallback = std::function<void(const PublisherInfoList&)>;
using RecurringRemoveCallback = std::function<void(Result)>;
using FetchIconCallback = std::function<void(bool, const std::string&)>;
using LoadURLCallback = std::function<void(bool, const std::string&,
    const std::map<std::string, std::string>& headers)>;
using OnRestoreCallback = std::function<void(bool)>;
using OnSaveCallback = std::function<void(const ledger::Result)>;
using OnLoadCallback = std::function<void(const ledger::Result, const std::string&)>;
using OnResetCallback = std::function<void(const ledger::Result)>;

class LEDGER_EXPORT LedgerClient {
 public:
  virtual ~LedgerClient() = default;

  // called when the wallet creation has completed
  virtual std::string GenerateGUID() const = 0;
  virtual void OnWalletInitialized(Result result) = 0;
  virtual void FetchWalletProperties() = 0;
  virtual void OnWalletProperties(Result result,
                                  std::unique_ptr<ledger::WalletInfo>) = 0;
  virtual void OnReconcileComplete(Result result,
                                   const std::string& viewing_id,
                                   ledger::REWARDS_CATEGORY category,
                                   const std::string& probi) = 0;

  virtual void LoadLedgerState(LedgerCallbackHandler* handler) = 0;
  virtual void SaveLedgerState(const std::string& ledger_state,
                               LedgerCallbackHandler* handler) = 0;

  virtual void LoadPublisherState(LedgerCallbackHandler* handler) = 0;
  virtual void SavePublisherState(const std::string& publisher_state,
                                  LedgerCallbackHandler* handler) = 0;

  virtual void SavePublishersList(const std::string& publisher_state,
    LedgerCallbackHandler* handler) = 0;
  virtual void LoadPublisherList(LedgerCallbackHandler* handler) = 0;


  virtual void LoadNicewareList(ledger::GetNicewareListCallback callback) = 0;

  virtual void SavePublisherInfo(std::unique_ptr<PublisherInfo> publisher_info,
                                PublisherInfoCallback callback) = 0;
  virtual void SaveActivityInfo(std::unique_ptr<PublisherInfo> publisher_info,
                                PublisherInfoCallback callback) = 0;
  virtual void LoadPublisherInfo(const std::string& publisher_key,
                                 PublisherInfoCallback callback) = 0;
  virtual void LoadActivityInfo(ActivityInfoFilter filter,
                                PublisherInfoCallback callback) = 0;
  virtual void LoadPanelPublisherInfo(ActivityInfoFilter filter,
                                      PublisherInfoCallback callback) = 0;
  virtual void LoadMediaPublisherInfo(const std::string& media_key,
                                PublisherInfoCallback callback) = 0;
  virtual void SaveMediaPublisherInfo(const std::string& media_key,
                                const std::string& publisher_id) = 0;
  virtual void GetActivityInfoList(uint32_t start, uint32_t limit,
                                    ActivityInfoFilter filter,
                                    PublisherInfoListCallback callback) = 0;

  // TODO this can be removed
  virtual void FetchGrant(const std::string& lang, const std::string& paymentId) = 0;
  virtual void OnGrant(ledger::Result result, const ledger::Grant& grant) = 0;
  virtual void GetGrantCaptcha() = 0;
  virtual void OnGrantCaptcha(const std::string& image, const std::string& hint) = 0;
  virtual void OnRecoverWallet(Result result, double balance, const std::vector<ledger::Grant>& grants) = 0;
  virtual void OnGrantFinish(ledger::Result result, const ledger::Grant& grant) = 0;
  virtual void OnPanelPublisherInfo(Result result,
                                   std::unique_ptr<ledger::PublisherInfo>,
                                   uint64_t windowId) = 0;
  virtual void OnExcludedSitesChanged(const std::string& publisher_id) = 0;
  virtual void FetchFavIcon(const std::string& url,
                            const std::string& favicon_key,
                            FetchIconCallback callback) = 0;
  virtual void SaveContributionInfo(const std::string& probi,
                                    const int month,
                                    const int year,
                                    const uint32_t date,
                                    const std::string& publisher_key,
                                    const ledger::REWARDS_CATEGORY category) = 0;
  virtual void GetRecurringDonations(ledger::PublisherInfoListCallback callback) = 0;
  virtual void OnRemoveRecurring(const std::string& publisher_key,
                                 ledger::RecurringRemoveCallback callback) = 0;

  //uint64_t time_offset (input): timer offset in seconds.
  //uint32_t timer_id (output) : 0 in case of failure
  virtual void SetTimer(uint64_t time_offset, uint32_t & timer_id) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual void LoadURL(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::URL_METHOD& method,
      ledger::LoadURLCallback callback) = 0;
  virtual void SetContributionAutoInclude(const std::string& publisher_key,
                                          bool excluded,
                                          uint64_t windowId) = 0;

  virtual void SavePendingContribution(
      const ledger::PendingContributionList& list) = 0;

  // Logs debug information
  virtual std::unique_ptr<LogStream> Log(
      const char* file,
      int line,
      const ledger::LogLevel log_level) const = 0;

  virtual std::unique_ptr<LogStream> VerboseLog(
      const char* file,
      int line,
      int vlog_level) const = 0;

  // Logs confirmations-related debug information
  virtual std::unique_ptr<confirmations::LogStream> LogConfirmations(
      const char* file,
      int line,
      const int log_level) const = 0;

  virtual void OnRestorePublishers(ledger::OnRestoreCallback callback) = 0;

  virtual void SaveNormalizedPublisherList(
    const ledger::PublisherInfoListStruct& normalized_list) = 0;

  virtual void SaveConfirmationsState(const std::string& name,
                                      const std::string& value,
                                      ledger::OnSaveCallback callback) = 0;
  virtual void LoadConfirmationsState(const std::string& name,
                                      ledger::OnLoadCallback callback) = 0;
  virtual void ResetConfirmationsState(const std::string& name,
                                       ledger::OnResetCallback callback) = 0;
  virtual uint32_t SetConfirmationsTimer(const uint64_t time_offset) = 0;
  virtual void KillConfirmationsTimer(uint32_t timer_id) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CLIENT_H_
