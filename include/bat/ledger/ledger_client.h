/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_CLIENT_H_
#define BAT_LEDGER_LEDGER_CLIENT_H_

#include <functional>
#include <memory>
#include <vector>

#include "bat/ledger/balance_report_info.h"
#include "bat/ledger/export.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/ledger_task_runner.h"
#include "bat/ledger/ledger_url_loader.h"
#include "bat/ledger/media_publisher_info.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/wallet_info.h"
#include "bat/ledger/grant.h"

namespace ledger {

LEDGER_EXPORT enum URL_METHOD {
  GET = 0,
  PUT = 1,
  POST = 2
};

using PublisherInfoCallback = std::function<void(Result,
    std::unique_ptr<PublisherInfo>)>;
using GetPublisherInfoListCallback =
    std::function<void(const PublisherInfoList&, uint32_t /* next_record */)>;

class LEDGER_EXPORT LedgerClient {
 public:
  virtual ~LedgerClient() = default;

  // called when the wallet creation has completed
  virtual std::string GenerateGUID() const = 0;
  virtual void OnWalletInitialized(Result result) = 0;
  virtual void GetWalletProperties() = 0;
  virtual void OnWalletProperties(Result result,
                                  std::unique_ptr<ledger::WalletInfo>) = 0;
  virtual void OnReconcileComplete(Result result,
                                   const std::string& viewing_id) = 0;

  virtual void LoadLedgerState(LedgerCallbackHandler* handler) = 0;
  virtual void SaveLedgerState(const std::string& ledger_state,
                               LedgerCallbackHandler* handler) = 0;

  virtual void LoadPublisherState(LedgerCallbackHandler* handler) = 0;
  virtual void SavePublisherState(const std::string& publisher_state,
                                  LedgerCallbackHandler* handler) = 0;

  virtual void SavePublishersList(const std::string& publisher_state,
    LedgerCallbackHandler* handler) = 0;
  virtual void LoadPublisherList(LedgerCallbackHandler* handler) = 0;



  virtual void SavePublisherInfo(std::unique_ptr<PublisherInfo> publisher_info,
                                PublisherInfoCallback callback) = 0;
  virtual void LoadPublisherInfo(PublisherInfoFilter filter,
                                PublisherInfoCallback callback) = 0;
  virtual void LoadMediaPublisherInfo(const std::string& media_key,
                                PublisherInfoCallback callback) = 0;
  virtual void SaveMediaPublisherInfo(const std::string& media_key,
                                const std::string& publisher_id) = 0;
  virtual void LoadPublisherInfoList(uint32_t start, uint32_t limit,
                                    PublisherInfoFilter filter,
                                    GetPublisherInfoListCallback callback) = 0;

  virtual void GetGrant(const std::string& lang, const std::string& paymentId) = 0;
  virtual void OnGrant(ledger::Result result, const ledger::Grant& grant) = 0;
  virtual void GetGrantCaptcha() = 0;
  virtual void OnGrantCaptcha(const std::string& image, const std::string& hint) = 0;
  virtual void OnRecoverWallet(Result result, double balance, const std::vector<ledger::Grant>& grants) = 0;
  virtual void OnGrantFinish(ledger::Result result, const ledger::Grant& grant) = 0;

  //uint64_t time_offset (input): timer offset in seconds.
  //uint32_t timer_id (output) : 0 in case of failure
  virtual void SetTimer(uint64_t time_offset, uint32_t & timer_id) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual std::unique_ptr<ledger::LedgerURLLoader> LoadURL(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::URL_METHOD& method,
      ledger::LedgerCallbackHandler* handler) = 0;
  // RunIOTask and RunTask are temporary workarounds for leveldb
  // and we should replace them with a ledger_client api for reading/writing
  // individual records
  virtual void RunIOTask(std::unique_ptr<LedgerTaskRunner> task) = 0;
  // If any callbacks are made from inside RunIOTask you must use
  // RunTask to return back to the calling thread
  virtual void RunTask(std::unique_ptr<LedgerTaskRunner> task) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CLIENT_H_
