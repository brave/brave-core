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

  void OnWalletProperties(
      ledger::Result result,
      ledger::WalletPropertiesPtr properties) override;
  void OnReconcileComplete(ledger::Result result,
                           const std::string& contribution_id,
                           const double amount,
                           const ledger::RewardsType type) override;
  void LoadLedgerState(ledger::OnLoadCallback callback) override;
  void LoadPublisherState(ledger::OnLoadCallback callback) override;
  void SaveLedgerState(
      const std::string& ledger_state,
      ledger::ResultCallback callback) override;
  void SavePublisherState(
      const std::string& publisher_state,
      ledger::ResultCallback callback) override;
  void SetTimer(uint64_t time_offset, uint32_t* timer_id) override;
  void KillTimer(const uint32_t timer_id) override;

  void LoadURL(const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::UrlMethod method,
      ledger::LoadURLCallback callback) override;

  void OnPanelPublisherInfo(ledger::Result result,
                            ledger::PublisherInfoPtr info,
                            uint64_t windowId) override;
  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::FetchIconCallback callback) override;

  std::unique_ptr<ledger::LogStream> Log(const char* file,
                                         int line,
                                         ledger::LogLevel level) const override;
  std::unique_ptr<ledger::LogStream> VerboseLog(const char* file,
                                         int line,
                                         int verbosity_level) const override;

  std::string URIEncode(const std::string& value) override;

  void PublisherListNormalized(ledger::PublisherInfoList list) override;

  void SaveState(const std::string& name,
                 const std::string& value,
                 ledger::ResultCallback callback) override;
  void LoadState(const std::string& name,
                 ledger::OnLoadCallback callback) override;
  void ResetState(const std::string& name,
                  ledger::ResultCallback callback) override;
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

  bool GetBooleanOption(const std::string& name) const override;
  int GetIntegerOption(const std::string& name) const override;
  double GetDoubleOption(const std::string& name) const override;
  std::string GetStringOption(const std::string& name) const override;
  int64_t GetInt64Option(const std::string& name) const override;
  uint64_t GetUint64Option(const std::string& name) const override;

  void SetConfirmationsIsReady(const bool is_ready) override;

  void ConfirmationsTransactionHistoryDidChange() override;

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
      ledger::ResultCallback callback) override;


  ledger::TransferFeeList GetTransferFees(
      const std::string& wallet_type) override;

  void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee) override;

  void RemoveTransferFee(
      const std::string& wallet_type,
      const std::string& id) override;

  ledger::ClientInfoPtr GetClientInfo() override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(
      ledger::DBTransactionPtr transaction,
      ledger::RunDBTransactionCallback callback) override;

  void GetCreateScript(
      ledger::GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const ledger::Result result) override;

 private:
  bool Connected() const;

  void LoadNicewareList(ledger::GetNicewareListCallback callback) override;

  mojom::BatLedgerClientAssociatedPtr bat_ledger_client_;

  void OnLoadLedgerState(ledger::OnLoadCallback callback,
      const ledger::Result result, const std::string& data);
  void OnLoadPublisherState(ledger::OnLoadCallback callback,
      const ledger::Result result, const std::string& data);
  void OnSavePublisherState(
      ledger::ResultCallback callback,
      const ledger::Result result);

  DISALLOW_COPY_AND_ASSIGN(BatLedgerClientMojoProxy);
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_CLIENT_MOJO_PROXY_H_
