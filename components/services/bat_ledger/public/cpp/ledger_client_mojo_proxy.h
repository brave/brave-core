/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_PROXY_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_PROXY_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace bat_ledger {

class LedgerClientMojoProxy : public mojom::BatLedgerClient,
                          public base::SupportsWeakPtr<LedgerClientMojoProxy> {
 public:
  explicit LedgerClientMojoProxy(ledger::LedgerClient* ledger_client);
  ~LedgerClientMojoProxy() override;

  // bat_ledger::mojom::BatLedgerClient
  void GenerateGUID(GenerateGUIDCallback callback) override;
  void LoadLedgerState(LoadLedgerStateCallback callback) override;
  void OnWalletInitialized(int32_t result) override;
  void OnWalletProperties(int32_t result, const std::string& info) override;
  void OnGrant(int32_t result, const std::string& grant) override;
  void OnGrantCaptcha(const std::string& image,
      const std::string& hint) override;
  void OnRecoverWallet(int32_t result, double balance,
      const std::vector<std::string>& grants) override;
  void OnReconcileComplete(int32_t result, const std::string& viewing_id,
      int32_t category, const std::string& probi) override;
  void OnGrantFinish(int32_t result, const std::string& grant) override;

  void LoadPublisherState(LoadPublisherStateCallback callback) override;
  void LoadPublisherList(LoadPublisherListCallback callback) override;
  void SaveLedgerState(const std::string& ledger_state,
      SaveLedgerStateCallback callback) override;
  void SavePublisherState(const std::string& publisher_state,
      SavePublisherStateCallback callback) override;
  void SavePublishersList(const std::string& publishers_list,
      SavePublishersListCallback callback) override;

  void SavePublisherInfo(const std::string& publisher_info,
      SavePublisherInfoCallback callback) override;
  void LoadPublisherInfo(const std::string& publisher_key,
      LoadPublisherInfoCallback callback) override;
  void LoadPanelPublisherInfo(const std::string& filter,
      LoadPanelPublisherInfoCallback callback) override;
  void LoadMediaPublisherInfo(const std::string& media_key,
      LoadMediaPublisherInfoCallback callback) override;

  void FetchFavIcon(const std::string& url, const std::string& favicon_key,
      FetchFavIconCallback callback) override;
  void GetRecurringTips(GetRecurringTipsCallback callback) override;

  void LoadNicewareList(LoadNicewareListCallback callback) override;
  void OnRemoveRecurring(const std::string& publisher_key,
      OnRemoveRecurringCallback callback) override;

  void SetTimer(uint64_t time_offset, SetTimerCallback callback) override;
  void KillTimer(const uint32_t timer_id) override;
  void OnPanelPublisherInfo(int32_t result, const std::string& info,
      uint64_t window_id) override;
  void OnExcludedSitesChanged(const std::string& publisher_id,
                              int exclude) override;
  void SaveContributionInfo(const std::string& probi, int32_t month,
      int32_t year, uint32_t date, const std::string& publisher_key,
      int32_t category) override;
  void SaveMediaPublisherInfo(const std::string& media_key,
      const std::string& publisher_id) override;
  void FetchGrants(const std::string& lang,
      const std::string& payment_id) override;
  void GetGrantCaptcha(
      const std::string& promotion_id,
      const std::string& promotion_type) override;

  void URIEncode(const std::string& value,
      URIEncodeCallback callback) override;

  void LoadURL(const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    int32_t method,
    LoadURLCallback callback) override;

  void SavePendingContribution(
      const std::string& list) override;

  void LoadActivityInfo(const std::string& filter,
      LoadActivityInfoCallback callback) override;

  void SaveActivityInfo(const std::string& publisher_info,
      SaveActivityInfoCallback callback) override;

  void OnRestorePublishers(OnRestorePublishersCallback callback) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           const std::string& filter,
                           GetActivityInfoListCallback callback) override;

  void SaveNormalizedPublisherList(
    const std::string& normalized_list) override;
  void SaveState(const std::string& name,
                              const std::string& value,
                              SaveStateCallback callback) override;
  void LoadState(const std::string& name,
                              LoadStateCallback callback) override;
  void ResetState(
      const std::string& name,
      ResetStateCallback callback) override;
  void SetConfirmationsIsReady(const bool is_ready) override;

  void ConfirmationsTransactionHistoryDidChange() override;

  void GetExcludedPublishersNumberDB(GetExcludedPublishersNumberDBCallback callback) override;

 private:
  // workaround to pass base::OnceCallback into std::bind
  // also serves as a wrapper for passing ledger::LedgerCallbackHandler*
  template <typename Callback>
  class CallbackHolder : public ledger::LedgerCallbackHandler {
   public:
    CallbackHolder(base::WeakPtr<LedgerClientMojoProxy> client,
        Callback callback)
        : client_(client),
          callback_(std::move(callback)) {}
    ~CallbackHolder() = default;
    bool is_valid() { return !!client_.get(); }
    Callback& get() { return callback_; }

    // ledger::LedgerCallbackHandler impl
    void OnLedgerStateLoaded(ledger::Result result,
        const std::string& data) override;
    void OnPublisherStateLoaded(ledger::Result result,
        const std::string& data) override;
    void OnPublisherListLoaded(ledger::Result result,
        const std::string& data) override;
    void OnLedgerStateSaved(ledger::Result result) override;
    void OnPublisherStateSaved(ledger::Result result) override;
    void OnPublishersListSaved(ledger::Result result) override;
   private:
    base::WeakPtr<LedgerClientMojoProxy> client_;
    Callback callback_;
  };

  static void OnSavePublisherInfo(
      CallbackHolder<SavePublisherInfoCallback>* holder,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info);

  static void OnLoadPublisherInfo(
      CallbackHolder<LoadPublisherInfoCallback>* holder,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info);

  static void OnLoadPanelPublisherInfo(
      CallbackHolder<LoadPanelPublisherInfoCallback>* holder,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info);

  static void OnLoadMediaPublisherInfo(
      CallbackHolder<LoadMediaPublisherInfoCallback>* holder,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info);

  static void OnFetchFavIcon(
      CallbackHolder<FetchFavIconCallback>* holder,
      bool success,
      const std::string& favicon_url);

  static void OnGetRecurringTips(
      CallbackHolder<GetRecurringTipsCallback>* holder,
      const ledger::PublisherInfoList& publisher_info_list,
      uint32_t next_record);

  static void OnLoadNicewareList(
      CallbackHolder<LoadNicewareListCallback>* holder,
      int32_t result, const std::string& data);

  static void OnRecurringRemoved(
      CallbackHolder<OnRemoveRecurringCallback>* holder,
      int32_t result);

  static void OnLoadURL(
      CallbackHolder<LoadURLCallback>* holder,
      int32_t response_code, const std::string& response,
      const std::map<std::string, std::string>& headers);

  static void OnLoadActivityInfo(
      CallbackHolder<LoadActivityInfoCallback>* holder,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info);

  static void OnSaveActivityInfo(
      CallbackHolder<SaveActivityInfoCallback>* holder,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info);

  static void OnRestorePublishersDone(
    CallbackHolder<OnRestorePublishersCallback>* holder,
    bool result);

  static void OnGetActivityInfoList(
      CallbackHolder<GetActivityInfoListCallback>* holder,
      const ledger::PublisherInfoList& publisher_info_list,
      uint32_t next_record);

  static void OnSaveState(
      CallbackHolder<SaveStateCallback>* holder,
      ledger::Result result);

  static void OnLoadState(
      CallbackHolder<LoadStateCallback>* holder,
      ledger::Result result,
      const std::string& value);

  static void OnResetState(
      CallbackHolder<ResetStateCallback>* holder,
      ledger::Result result);

  static void OnGetExcludedPublishersNumberDB(
      CallbackHolder<GetExcludedPublishersNumberDBCallback>* holder,
      uint32_t number);

  ledger::LedgerClient* ledger_client_;

  DISALLOW_COPY_AND_ASSIGN(LedgerClientMojoProxy);
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_PROXY_H_
