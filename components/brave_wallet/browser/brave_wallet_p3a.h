/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace brave_wallet {

inline constexpr char kKeyringCreatedHistogramName[] =
    "Brave.Wallet.KeyringCreated";
inline constexpr char kOnboardingConversionHistogramName[] =
    "Brave.Wallet.OnboardingConversion.3";
inline constexpr char kNewUserBalanceHistogramName[] =
    "Brave.Wallet.NewUserBalance";
inline constexpr char kEthProviderHistogramName[] =
    "Brave.Wallet.EthProvider.4";
inline constexpr char kSolProviderHistogramName[] =
    "Brave.Wallet.SolProvider.2";
inline constexpr char kEthTransactionSentHistogramName[] =
    "Brave.Wallet.EthTransactionSent";
inline constexpr char kSolTransactionSentHistogramName[] =
    "Brave.Wallet.SolTransactionSent";
inline constexpr char kFilTransactionSentHistogramName[] =
    "Brave.Wallet.FilTransactionSent";
inline constexpr char kBtcTransactionSentHistogramName[] =
    "Brave.Wallet.BtcTransactionSent";
inline constexpr char kZecTransactionSentHistogramName[] =
    "Brave.Wallet.ZecTransactionSent";
inline constexpr char kEthActiveAccountHistogramName[] =
    "Brave.Wallet.ActiveEthAccounts";
inline constexpr char kSolActiveAccountHistogramName[] =
    "Brave.Wallet.ActiveSolAccounts";
inline constexpr char kFilActiveAccountHistogramName[] =
    "Brave.Wallet.ActiveFilAccounts";
inline constexpr char kBtcActiveAccountHistogramName[] =
    "Brave.Wallet.ActiveBtcAccounts";
inline constexpr char kZecActiveAccountHistogramName[] =
    "Brave.Wallet.ActiveZecAccounts";
inline constexpr char kBraveWalletDailyHistogramName[] =
    "Brave.Wallet.UsageDaily";
inline constexpr char kBraveWalletWeeklyHistogramName[] =
    "Brave.Wallet.UsageWeekly";
inline constexpr char kBraveWalletMonthlyHistogramName[] =
    "Brave.Wallet.UsageMonthly";
inline constexpr char kBraveWalletNewUserReturningHistogramName[] =
    "Brave.Wallet.NewUserReturning";
inline constexpr char kBraveWalletLastUsageTimeHistogramName[] =
    "Brave.Wallet.LastUsageTime";
inline constexpr char kBraveWalletNFTCountHistogramName[] =
    "Brave.Wallet.NFTCount";
inline constexpr char kBraveWalletNFTNewUserHistogramName[] =
    "Brave.Wallet.NFTNewUser";
inline constexpr char kBraveWalletNFTDiscoveryEnabledHistogramName[] =
    "Brave.Wallet.NFTDiscoveryEnabled";

class BraveWalletService;
class KeyringService;
class TxService;

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
                       public mojom::BraveWalletP3A,
                       public mojom::TxServiceObserver {
 public:
  BraveWalletP3A(BraveWalletService* wallet_service,
                 KeyringService* keyring_service,
                 TxService* tx_service,
                 PrefService* profile_prefs,
                 PrefService* local_state);

  // For testing
  BraveWalletP3A();

  ~BraveWalletP3A() override;
  BraveWalletP3A(const BraveWalletP3A&) = delete;
  BraveWalletP3A& operator=(BraveWalletP3A&) = delete;

  void Bind(mojo::PendingReceiver<mojom::BraveWalletP3A> receiver);

  void AddObservers();

  void ReportUsage(bool unlocked);
  void ReportJSProvider(mojom::JSProviderType provider_type,
                        mojom::CoinType coin_type,
                        bool allow_provider_overwrite) override;
  void ReportOnboardingAction(mojom::OnboardingAction action) override;
  void RecordActiveWalletCount(int count, mojom::CoinType coin_type) override;
  void RecordNFTGalleryView(int nft_count) override;

  // KeyringServiceObserverBase:
  void WalletCreated() override;

  // mojom::TxServiceObserver:
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnTxServiceReset() override {}

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWalletP3AUnitTest, ReportTransactionSent);
  friend class BraveWalletP3AUnitTest;

  void OnUpdateTimerFired();
  void WriteUsageStatsToHistogram();
  void RecordInitialBraveWalletP3AState();
  std::optional<mojom::OnboardingAction> GetLastOnboardingAction();
  void RecordOnboardingHistogram();
  void MaybeRecordNewUserBalance();
  void ReportNftDiscoverySetting();
  void ReportTransactionSent(mojom::CoinType coin, bool new_send);
  raw_ptr<BraveWalletService> wallet_service_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<TxService> tx_service_;
  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<PrefService> local_state_;

  mojo::Receiver<mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};

  mojo::Receiver<mojom::TxServiceObserver> tx_service_observer_receiver_{this};

  base::OneShotTimer onboarding_report_timer_;

  mojo::ReceiverSet<mojom::BraveWalletP3A> receivers_;
  base::RepeatingTimer update_timer_;
  PrefChangeRegistrar local_state_change_registrar_;
  PrefChangeRegistrar profile_pref_change_registrar_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
