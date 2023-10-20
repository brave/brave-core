/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
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
extern const char kNewUserBalanceHistogramName[];
extern const char kEthProviderHistogramName[];
extern const char kSolProviderHistogramName[];
extern const char kEthTransactionSentHistogramName[];
extern const char kSolTransactionSentHistogramName[];
extern const char kFilTransactionSentHistogramName[];
extern const char kEthActiveAccountHistogramName[];
extern const char kSolActiveAccountHistogramName[];
extern const char kFilActiveAccountHistogramName[];
extern const char kBraveWalletDailyHistogramName[];
extern const char kBraveWalletWeeklyHistogramName[];
extern const char kBraveWalletMonthlyHistogramName[];
extern const char kBraveWalletNewUserReturningHistogramName[];
extern const char kBraveWalletNFTCountHistogramName[];
extern const char kBraveWalletNFTNewUserHistogramName[];
extern const char kBraveWalletNFTDiscoveryEnabledHistogramName[];

class BraveWalletService;
class KeyringService;

enum class JSProviderAnswer {
  kNoWallet = 0,
  kWalletDisabled = 1,
  kNativeNotOverridden = 2,
  kNativeOverridingDisallowed = 3,
  kThirdPartyNotOverriding = 4,
  kThirdPartyOverriding = 5,
  kMaxValue = kThirdPartyOverriding
};

// Reports BraveWallet related P3A data
class BraveWalletP3A : public KeyringServiceObserverBase,
                       public mojom::BraveWalletP3A {
 public:
  BraveWalletP3A(BraveWalletService* wallet_service,
                 KeyringService* keyring_service,
                 PrefService* profile_prefs,
                 PrefService* local_state);

  // For testing
  BraveWalletP3A();

  ~BraveWalletP3A() override;
  BraveWalletP3A(const BraveWalletP3A&) = delete;
  BraveWalletP3A& operator=(BraveWalletP3A&) = delete;

  mojo::PendingRemote<mojom::BraveWalletP3A> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveWalletP3A> receiver);

  void AddObservers();

  void ReportUsage(bool unlocked);
  void ReportJSProvider(mojom::JSProviderType provider_type,
                        mojom::CoinType coin_type,
                        bool allow_provider_overwrite) override;
  void ReportOnboardingAction(mojom::OnboardingAction action) override;
  void ReportTransactionSent(mojom::CoinType coin, bool new_send) override;
  void RecordActiveWalletCount(int count, mojom::CoinType coin_type) override;
  void RecordNFTGalleryView(int nft_count) override;

  // KeyringServiceObserverBase:
  void WalletCreated() override;

 private:
  void MigrateUsageProfilePrefsToLocalState();
  void OnUpdateTimerFired();
  void WriteUsageStatsToHistogram();
  void RecordInitialBraveWalletP3AState();
  absl::optional<mojom::OnboardingAction> GetLastOnboardingAction();
  void RecordOnboardingHistogram();
  void MaybeRecordNewUserBalance();
  void ReportNftDiscoverySetting();
  raw_ptr<BraveWalletService> wallet_service_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<PrefService> local_state_;

  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};

  base::OneShotTimer onboarding_report_timer_;

  mojo::ReceiverSet<mojom::BraveWalletP3A> receivers_;
  base::RepeatingTimer update_timer_;
  PrefChangeRegistrar local_state_change_registrar_;
  PrefChangeRegistrar profile_pref_change_registrar_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
