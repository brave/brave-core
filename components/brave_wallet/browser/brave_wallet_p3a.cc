/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"

#include <utility>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

const char kDefaultWalletHistogramName[] = "Brave.Wallet.DefaultWalletSetting";
const char kDefaultSolanaWalletHistogramName[] =
    "Brave.Wallet.DefaultSolanaWalletSetting";
const char kKeyringCreatedHistogramName[] = "Brave.Wallet.KeyringCreated";
const char kOnboardingConversionHistogramName[] =
    "Brave.Wallet.OnboardingConversion.2";
const char kEthProviderHistogramName[] = "Brave.Wallet.EthProvider";
const char kEthTransactionSentHistogramName[] =
    "Brave.Wallet.EthTransactionSent";
const char kSolTransactionSentHistogramName[] =
    "Brave.Wallet.SolTransactionSent";
const char kFilTransactionSentHistogramName[] =
    "Brave.Wallet.FilTransactionSent";
const char kEthActiveAccountHistogramName[] = "Brave.Wallet.ActiveEthAccounts";
const char kSolActiveAccountHistogramName[] = "Brave.Wallet.ActiveSolAccounts";
const char kFilActiveAccountHistogramName[] = "Brave.Wallet.ActiveFilAccounts";

namespace {

constexpr int kActiveAccountBuckets[] = {0, 1, 2, 3, 7};

// Has the Wallet keyring been created?
// 0) No, 1) Yes
void RecordKeyringCreated(mojom::KeyringInfoPtr keyring_info) {
  UMA_HISTOGRAM_BOOLEAN(kKeyringCreatedHistogramName,
                        static_cast<int>(keyring_info->is_keyring_created));
}

// What is the DefaultWalletSetting (Ethereum)?
// 0) AskDeprecated, 1) None, 2) CryptoWallets,
// 3) BraveWalletPreferExtension, 4) BraveWallet
void RecordDefaultEthereumWalletSetting(PrefService* pref_service) {
  const int max_bucket =
      static_cast<int>(brave_wallet::mojom::DefaultWallet::kMaxValue);
  auto default_wallet = pref_service->GetInteger(kDefaultEthereumWallet);
  UMA_HISTOGRAM_EXACT_LINEAR(kDefaultWalletHistogramName, default_wallet,
                             max_bucket);
}

// What is the DefaultSolanaWalletSetting?
// 0) AskDeprecated, 1) None, 2) CryptoWallets,
// 3) BraveWalletPreferExtension, 4) BraveWallet
void RecordDefaultSolanaWalletSetting(PrefService* pref_service) {
  const int max_bucket =
      static_cast<int>(brave_wallet::mojom::DefaultWallet::kMaxValue);
  auto default_wallet = pref_service->GetInteger(kDefaultSolanaWallet);
  UMA_HISTOGRAM_EXACT_LINEAR(kDefaultSolanaWalletHistogramName, default_wallet,
                             max_bucket);
}

}  // namespace

BraveWalletP3A::BraveWalletP3A(BraveWalletService* wallet_service,
                               KeyringService* keyring_service,
                               PrefService* pref_service)
    : wallet_service_(wallet_service),
      keyring_service_(keyring_service),
      pref_service_(pref_service) {
  RecordInitialBraveWalletP3AState();
  AddObservers();
}

BraveWalletP3A::~BraveWalletP3A() = default;

void BraveWalletP3A::AddObservers() {
  wallet_service_->AddObserver(
      wallet_service_observer_receiver_.BindNewPipeAndPassRemote());
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

mojo::PendingRemote<mojom::BraveWalletP3A> BraveWalletP3A::MakeRemote() {
  mojo::PendingRemote<mojom::BraveWalletP3A> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveWalletP3A::Bind(
    mojo::PendingReceiver<mojom::BraveWalletP3A> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveWalletP3A::Update() {
  ReportTransactionSent(mojom::CoinType::ETH, false);
  ReportTransactionSent(mojom::CoinType::FIL, false);
  ReportTransactionSent(mojom::CoinType::SOL, false);
}

void BraveWalletP3A::ReportEthereumProvider(
    mojom::EthereumProviderType provider_type) {
  UMA_HISTOGRAM_ENUMERATION(kEthProviderHistogramName, provider_type);
}

void BraveWalletP3A::ReportOnboardingAction(
    mojom::OnboardingAction onboarding_action) {
  UMA_HISTOGRAM_ENUMERATION(kOnboardingConversionHistogramName,
                            onboarding_action);
}

void BraveWalletP3A::ReportTransactionSent(mojom::CoinType coin,
                                           bool new_send) {
  DictionaryPrefUpdate last_sent_time_update(
      pref_service_, kBraveWalletLastTransactionSentTimeDict);
  base::Value::Dict& last_sent_time_dict = last_sent_time_update->GetDict();

  std::string coin_key = base::NumberToString(static_cast<int>(coin));

  base::Time now = base::Time::Now();
  base::Time last_sent_time = base::Time::FromDoubleT(
      last_sent_time_dict.FindDouble(coin_key).value_or(0.0));

  if (!new_send && last_sent_time.is_null()) {
    // Don't report if a transaction was never sent.
    return;
  }
  int answer = 0;
  if (new_send || (now - last_sent_time) < base::Days(7)) {
    answer = 1;
  }
  if (new_send) {
    last_sent_time_dict.Set(coin_key, now.ToDoubleT());
  }

  const char* histogram_name;
  switch (coin) {
    case mojom::CoinType::ETH:
      histogram_name = kEthTransactionSentHistogramName;
      break;
    case mojom::CoinType::SOL:
      histogram_name = kSolTransactionSentHistogramName;
      break;
    case mojom::CoinType::FIL:
      histogram_name = kFilTransactionSentHistogramName;
      break;
  }

  base::UmaHistogramExactLinear(histogram_name, answer, 2);
}

void BraveWalletP3A::RecordActiveWalletCount(int count,
                                             mojom::CoinType coin_type) {
  DCHECK_GE(count, 0);
  const char* histogram_name;

  switch (coin_type) {
    case mojom::CoinType::ETH:
      histogram_name = kEthActiveAccountHistogramName;
      break;
    case mojom::CoinType::SOL:
      histogram_name = kSolActiveAccountHistogramName;
      break;
    case mojom::CoinType::FIL:
      histogram_name = kFilActiveAccountHistogramName;
      break;
    default:
      return;
  }

  const base::Value::Dict& active_wallet_dict =
      pref_service_->GetDict(kBraveWalletP3AActiveWalletDict);
  std::string coin_type_str = base::NumberToString(static_cast<int>(coin_type));
  if (!active_wallet_dict.FindBool(coin_type_str).has_value()) {
    if (count == 0) {
      // Should not record zero to histogram if user never had an active
      // account, to avoid sending unnecessary data.
      return;
    }
    DictionaryPrefUpdate active_wallet_dict_update(
        pref_service_, kBraveWalletP3AActiveWalletDict);
    active_wallet_dict_update->GetDict().Set(coin_type_str, true);
  }
  p3a_utils::RecordToHistogramBucket(histogram_name, kActiveAccountBuckets,
                                     count);
}

void BraveWalletP3A::RecordInitialBraveWalletP3AState() {
  keyring_service_->GetKeyringInfo(mojom::kDefaultKeyringId,
                                   base::BindOnce(&RecordKeyringCreated));
  RecordDefaultEthereumWalletSetting(pref_service_);
  RecordDefaultSolanaWalletSetting(pref_service_);
}

// KeyringServiceObserver
void BraveWalletP3A::KeyringCreated(const std::string& keyring_id) {
  keyring_service_->GetKeyringInfo(keyring_id,
                                   base::BindOnce(&RecordKeyringCreated));
}

// BraveWalletServiceObserver
void BraveWalletP3A::OnDefaultEthereumWalletChanged(
    brave_wallet::mojom::DefaultWallet default_wallet) {
  RecordDefaultEthereumWalletSetting(pref_service_);
}

// BraveWalletServiceObserver
void BraveWalletP3A::OnDefaultSolanaWalletChanged(
    brave_wallet::mojom::DefaultWallet default_wallet) {
  RecordDefaultSolanaWalletSetting(pref_service_);
}

}  // namespace brave_wallet
