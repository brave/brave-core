/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_CLIENT_MOJO_PROXY_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_CLIENT_MOJO_PROXY_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"

class SkBitmap;

namespace bat_ledger {

class BatLedgerClientMojoProxy : public ledger::LedgerClient,
                      public base::SupportsWeakPtr<BatLedgerClientMojoProxy> {
 public:
  BatLedgerClientMojoProxy(
      mojom::BatLedgerClientAssociatedPtrInfo client_info);
  ~BatLedgerClientMojoProxy() override;

  std::string GenerateGUID() const override;
  void OnWalletInitialized(ledger::Result result) override;
  void OnWalletProperties(
      ledger::Result result,
      ledger::WalletPropertiesPtr properties) override;
  void OnGrant(ledger::Result result,
               ledger::GrantPtr grant) override;
  void OnGrantCaptcha(const std::string& image,
                      const std::string& hint) override;
  void OnRecoverWallet(ledger::Result result,
                      double balance,
                      std::vector<ledger::GrantPtr> grants) override;
  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id,
                           ledger::REWARDS_CATEGORY category,
                           const std::string& probi) override;
  void OnGrantFinish(ledger::Result result,
                     ledger::GrantPtr grant) override;
  void LoadLedgerState(ledger::OnLoadCallback callback) override;
  void LoadPublisherState(ledger::OnLoadCallback callback) override;
  void SaveLedgerState(const std::string& ledger_state,
                       ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherState(const std::string& publisher_state,
                          ledger::LedgerCallbackHandler* handler) override;

  void SavePublisherInfo(ledger::PublisherInfoPtr publisher_info,
                         ledger::PublisherInfoCallback callback) override;
  void LoadPublisherInfo(const std::string& publisher_key,
                         ledger::PublisherInfoCallback callback) override;
  void LoadPanelPublisherInfo(ledger::ActivityInfoFilter filter,
                              ledger::PublisherInfoCallback callback) override;
  void SavePublishersList(const std::string& publishers_list,
                          ledger::LedgerCallbackHandler* handler) override;
  void SetTimer(uint64_t time_offset, uint32_t* timer_id) override;
  void KillTimer(const uint32_t timer_id) override;
  void LoadPublisherList(ledger::LedgerCallbackHandler* handler) override;

  void LoadURL(const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::URL_METHOD method,
      ledger::LoadURLCallback callback) override;

  void OnExcludedSitesChanged(const std::string& publisher_id,
                              ledger::PUBLISHER_EXCLUDE exclude) override;
  void OnPanelPublisherInfo(ledger::Result result,
                            ledger::PublisherInfoPtr info,
                            uint64_t windowId) override;
  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::FetchIconCallback callback) override;
  void SaveContributionInfo(const std::string& probi,
                            const int month,
                            const int year,
                            const uint32_t date,
                            const std::string& publisher_key,
                            const ledger::REWARDS_CATEGORY category) override;
  void GetRecurringTips(ledger::PublisherInfoListCallback callback) override;
  void GetOneTimeTips(ledger::PublisherInfoListCallback callback) override;
  std::unique_ptr<ledger::LogStream> Log(const char* file,
                                         int line,
                                         ledger::LogLevel level) const override;
  std::unique_ptr<ledger::LogStream> VerboseLog(const char* file,
                                         int line,
                                         int verbosity_level) const override;
  void LoadMediaPublisherInfo(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback) override;
  void SaveMediaPublisherInfo(const std::string& media_key,
                              const std::string& publisher_id) override;

  void FetchGrants(const std::string& lang,
                   const std::string& payment_id) override;

  std::string URIEncode(const std::string& value) override;

  void SavePendingContribution(
      ledger::PendingContributionList list) override;

  void LoadActivityInfo(ledger::ActivityInfoFilter filter,
                        ledger::PublisherInfoCallback callback) override;

  void SaveActivityInfo(ledger::PublisherInfoPtr publisher_info,
                        ledger::PublisherInfoCallback callback) override;

  void OnRestorePublishers(ledger::OnRestoreCallback callback) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           ledger::ActivityInfoFilter filter,
                           ledger::PublisherInfoListCallback callback) override;

  void SaveNormalizedPublisherList(
      ledger::PublisherInfoList normalized_list) override;

  void SaveState(const std::string& name,
                 const std::string& value,
                 ledger::OnSaveCallback callback) override;
  void LoadState(const std::string& name,
                 ledger::OnLoadCallback callback) override;
  void ResetState(const std::string& name,
                  ledger::OnResetCallback callback) override;
  void SetBooleanState(const std::string& name, bool value) override;
  bool GetBooleanState(const std::string& name) const override;
  void SetIntegerState(const std::string& name, int value) override;
  int GetIntegerState(const std::string& name) const override;
  void SetDoubleState(const std::string& name, double value) override;
  double GetDoubleState(const std::string& name) const override;
  void SetStringState(const std::string& name,
                              const std::string& value) override;
  std::string GetStringState(const std::string& name) const override;
  void SetInt64State(const std::string& name, int64_t value) override;
  int64_t GetInt64State(const std::string& name) const override;
  void SetUint64State(const std::string& name, uint64_t value) override;
  uint64_t GetUint64State(const std::string& name) const override;
  void ClearState(const std::string& name) override;


  void SetConfirmationsIsReady(const bool is_ready) override;

  void ConfirmationsTransactionHistoryDidChange() override;

  void GetPendingContributions(
      const ledger::PendingContributionInfoListCallback& callback) override;

  void RemovePendingContribution(
    const std::string& publisher_key,
    const std::string& viewing_id,
    uint64_t added_date,
    const ledger::RemovePendingContributionCallback& callback) override;

  void RemoveAllPendingContributions(
    const ledger::RemovePendingContributionCallback& callback) override;

  void GetPendingContributionsTotal(
    const ledger::PendingContributionsTotalCallback& callback) override;

  void OnContributeUnverifiedPublishers(
      ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;

  void GetExternalWallets(ledger::GetExternalWalletsCallback callback) override;

  void SaveExternalWallet(const std::string& wallet_type,
                           ledger::ExternalWalletPtr wallet) override;

  void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      const ledger::ShowNotificationCallback& callback) override;

 private:
  bool Connected() const;

  void LoadNicewareList(ledger::GetNicewareListCallback callback) override;
  void OnRemoveRecurring(const std::string& publisher_key,
      ledger::RecurringRemoveCallback callback) override;

  mojom::BatLedgerClientAssociatedPtr bat_ledger_client_;

  void OnLoadLedgerState(ledger::OnLoadCallback callback,
      int32_t result, const std::string& data);
  void OnLoadPublisherState(ledger::OnLoadCallback callback,
      int32_t result, const std::string& data);
  void OnLoadPublisherList(ledger::LedgerCallbackHandler* handler,
      int32_t result, const std::string& data);
  void OnSaveLedgerState(ledger::LedgerCallbackHandler* handler,
      int32_t result);
  void OnSavePublisherState(ledger::LedgerCallbackHandler* handler,
      int32_t result);
  void OnSavePublishersList(ledger::LedgerCallbackHandler* handler,
      int32_t result);

  DISALLOW_COPY_AND_ASSIGN(BatLedgerClientMojoProxy);
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_CLIENT_MOJO_PROXY_H_
