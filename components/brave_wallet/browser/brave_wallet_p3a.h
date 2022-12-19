/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace brave_wallet {

extern const char kDefaultWalletHistogramName[];
extern const char kDefaultSolanaWalletHistogramName[];
extern const char kKeyringCreatedHistogramName[];
extern const char kOnboardingConversionHistogramName[];
extern const char kEthProviderHistogramName[];
extern const char kEthTransactionSentHistogramName[];
extern const char kSolTransactionSentHistogramName[];
extern const char kFilTransactionSentHistogramName[];
extern const char kEthActiveAccountHistogramName[];
extern const char kSolActiveAccountHistogramName[];
extern const char kFilActiveAccountHistogramName[];
extern const char kBraveWalletWeeklyHistogramName[];
extern const char kBraveWalletMonthlyHistogramName[];
extern const char kBraveWalletNewUserReturningHistogramName[];

class BraveWalletService;
class KeyringService;

// Reports BraveWallet related P3A data
class BraveWalletP3A : public mojom::BraveWalletServiceObserver,
                       public mojom::KeyringServiceObserver,
                       public mojom::BraveWalletP3A {
 public:
  BraveWalletP3A(BraveWalletService* wallet_service,
                 KeyringService* keyring_service,
                 PrefService* profile_prefs,
                 PrefService* local_state);

  ~BraveWalletP3A() override;
  BraveWalletP3A(const BraveWalletP3A&) = delete;
  BraveWalletP3A& operator=(BraveWalletP3A&) = delete;

  mojo::PendingRemote<mojom::BraveWalletP3A> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveWalletP3A> receiver);

  void AddObservers();

  void ReportUsage(bool unlocked);
  void ReportEthereumProvider(
      mojom::EthereumProviderType provider_type) override;
  void ReportOnboardingAction(
      mojom::OnboardingAction onboarding_action) override;
  void ReportTransactionSent(mojom::CoinType coin, bool new_send) override;
  void RecordActiveWalletCount(int count, mojom::CoinType coin_type) override;

  // KeyringServiceObserver
  void KeyringCreated(const std::string& keyring_id) override;
  void KeyringRestored(const std::string& keyring_id) override {}
  void KeyringReset() override {}
  void Locked() override {}
  void Unlocked() override {}
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AccountsAdded(mojom::CoinType coin,
                     const std::vector<std::string>& addresses) override {}
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged(mojom::CoinType coin) override {}

  // BraveWalletServiceObserver
  void OnActiveOriginChanged(mojom::OriginInfoPtr origin_info) override {}
  void OnDefaultEthereumWalletChanged(
      brave_wallet::mojom::DefaultWallet wallet) override;
  void OnDefaultSolanaWalletChanged(
      brave_wallet::mojom::DefaultWallet wallet) override;
  void OnDefaultBaseCurrencyChanged(const std::string& currency) override {}
  void OnDefaultBaseCryptocurrencyChanged(
      const std::string& cryptocurrency) override {}
  void OnNetworkListChanged() override {}
  void OnDiscoverAssetsCompleted(
      std::vector<mojom::BlockchainTokenPtr> discovered_assets) override {}

 private:
  void MigrateUsageProfilePrefsToLocalState();
  void OnUpdateTimerFired();
  void WriteUsageStatsToHistogram(base::Time wallet_last_used,
                                  base::Time first_p3a_used,
                                  base::Time last_p3a_report,
                                  unsigned use_days_in_week);
  void RecordInitialBraveWalletP3AState();
  raw_ptr<BraveWalletService> wallet_service_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<PrefService> local_state_;

  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      wallet_service_observer_receiver_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};

  mojo::ReceiverSet<mojom::BraveWalletP3A> receivers_;
  base::RepeatingTimer update_timer_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
