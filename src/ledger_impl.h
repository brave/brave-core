/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_IMPL_H_
#define BAT_LEDGER_LEDGER_IMPL_H_

#include <memory>
#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/ledger_url_loader.h"
#include "bat_helper.h"
#include "ledger_task_runner_impl.h"
#include "url_request_handler.h"

namespace braveledger_bat_client {
class BatClient;
}

namespace braveledger_bat_get_media {
class BatGetMedia;
}

namespace braveledger_bat_publishers {
class BatPublishers;
}

namespace bat_ledger {

class LedgerImpl : public ledger::Ledger,
                   public ledger::LedgerCallbackHandler {
 public:
  LedgerImpl(ledger::LedgerClient* client);
  ~LedgerImpl() override;

  // Not copyable, not assignable
  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  std::string GenerateGUID() const;
  void Reconcile() override;
  void CreateWallet() override;

  void SetPublisherInfo(std::unique_ptr<ledger::PublisherInfo> publisher_info,
                        ledger::PublisherInfoCallback callback) override;
  void GetPublisherInfo(const ledger::PublisherInfo::id_type& publisher_id,
                        ledger::PublisherInfoCallback callback) override;
  void GetPublisherInfoList(uint32_t start, uint32_t limit,
                            ledger::PublisherInfoFilter filter,
                            ledger::GetPublisherInfoListCallback callback) override;

  void SetPublisherMinVisitTime(uint64_t duration_in_milliseconds) override;
  void SetPublisherMinVisits(unsigned int visits) override;
  void SetPublisherAllowNonVerified(bool allow) override;
  void SetContributionAmount(double amount) override;
  const std::string& GetBATAddress() const override;
  const std::string& GetBTCAddress() const override;
  const std::string& GetETHAddress() const override;
  const std::string& GetLTCAddress() const override;
  uint64_t GetPublisherMinVisitTime() const override; // In milliseconds
  unsigned int GetPublisherMinVisits() const override;
  bool GetPublisherAllowNonVerified() const override;
  double GetContributionAmount() const override;

  void SaveLedgerState(const std::string& data,
                       ledger::LedgerCallbackHandler* handler);
  void SavePublisherState(const std::string& data,
                          ledger::LedgerCallbackHandler* handler);
  void LoadLedgerState(ledger::LedgerCallbackHandler* handler);
  void LoadPublisherState(ledger::LedgerCallbackHandler* handler);

  void OnWalletCreated(ledger::Result);
  void OnWalletProperties(const braveledger_bat_helper::WALLET_PROPERTIES_ST&);
  void GetWalletProperties() const override;

  std::unique_ptr<ledger::LedgerURLLoader> LoadURL(const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::URL_METHOD& method,
      ledger::LedgerCallbackHandler* handler);
  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id);
  void RunIOTask(LedgerTaskRunnerImpl::Task task);
  void RunTask(LedgerTaskRunnerImpl::Task task);

 private:
  void OnLoad(const ledger::VisitData& visit_data) override;
  void OnUnload(uint32_t tab_id) override;
  void OnShow(uint32_t tab_id) override;
  void OnHide(uint32_t tab_id) override;
  void OnForeground(uint32_t tab_id) override;
  void OnBackground(uint32_t tab_id) override;
  void OnMediaStart(uint32_t tab_id) override;
  void OnMediaStop(uint32_t tab_id) override;
  void OnXHRLoad(uint32_t tab_id, const std::string& url) override;

  void initSynopsis();
  void OnSetPublisherInfo(ledger::PublisherInfoCallback callback,
                          ledger::Result result,
                          std::unique_ptr<ledger::PublisherInfo> info);

  void processMedia(const std::map<std::string,
                    std::string>& parts,
                    const std::string& type);
  void saveVisitCallback(const std::string& publisher,
                         uint64_t verifiedTimestamp);
  void OnMediaRequest(const std::string& url,
                      const std::string& urlQuery,
                      const std::string& type);

  void OnMediaRequestCallback(uint64_t duration,
                              const braveledger_bat_helper::MEDIA_PUBLISHER_INFO& mediaPublisherInfo);

  // LedgerCallbackHandler impl
  void OnLedgerStateLoaded(ledger::Result result,
                           const std::string& data) override;

  ledger::LedgerClient* ledger_client_;
  std::unique_ptr<braveledger_bat_client::BatClient> bat_client_;
  std::unique_ptr<braveledger_bat_publishers::BatPublishers> bat_publishers_;
  std::unique_ptr<braveledger_bat_get_media::BatGetMedia> bat_get_media_;

  URLRequestHandler handler_;
 };
}  // namespace bat_ledger

#endif  // BAT_LEDGER_LEDGER_IMPL_H_
