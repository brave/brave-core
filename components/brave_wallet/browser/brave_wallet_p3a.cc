/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"

#include <optional>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {

const int kRefreshP3AFrequencyHours = 24;
const int kActiveAccountBuckets[] = {0, 1, 2, 3, 7};
const char* kTimePrefsToMigrateToLocalState[] = {kBraveWalletLastUnlockTime,
                                                 kBraveWalletP3AFirstUnlockTime,
                                                 kBraveWalletP3ALastUnlockTime};
const char* kTimePrefsToRemove[] = {kBraveWalletP3AFirstReportTimeDeprecated,
                                    kBraveWalletP3ALastReportTimeDeprecated};
const int kNFTCountBuckets[] = {0, 4, 20};
constexpr base::TimeDelta kOnboardingRecordDelay = base::Seconds(120);

// Has the Wallet keyring been created?
// 0) No, 1) Yes
void RecordKeyringCreated(bool created) {
  UMA_HISTOGRAM_BOOLEAN(kKeyringCreatedHistogramName,
                        static_cast<int>(created));
}

}  // namespace

BraveWalletP3A::BraveWalletP3A(BraveWalletService* wallet_service,
                               KeyringService* keyring_service,
                               TxService* tx_service,
                               PrefService* profile_prefs,
                               PrefService* local_state)
    : wallet_service_(wallet_service),
      keyring_service_(keyring_service),
      tx_service_(tx_service),
      profile_prefs_(profile_prefs),
      local_state_(local_state) {
  DCHECK(profile_prefs);
  DCHECK(local_state);

  MigrateUsageProfilePrefsToLocalState();

  RecordInitialBraveWalletP3AState();
  AddObservers();

  local_state_change_registrar_.Init(local_state_);
  local_state_change_registrar_.Add(
      kBraveWalletLastUnlockTime,
      base::BindRepeating(&BraveWalletP3A::ReportUsage, base::Unretained(this),
                          true));
  profile_pref_change_registrar_.Init(profile_prefs_);
  profile_pref_change_registrar_.Add(
      kBraveWalletNftDiscoveryEnabled,
      base::BindRepeating(&BraveWalletP3A::ReportNftDiscoverySetting,
                          base::Unretained(this)));

  // try to record the onboarding histogram
  // just in the case the user quit the app
  // before the 120 second deadline in the last
  // app session
  RecordOnboardingHistogram();

  ReportNftDiscoverySetting();
}

BraveWalletP3A::BraveWalletP3A() = default;

BraveWalletP3A::~BraveWalletP3A() = default;

void BraveWalletP3A::AddObservers() {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
  tx_service_->AddObserver(
      tx_service_observer_receiver_.BindNewPipeAndPassRemote());
  update_timer_.Start(FROM_HERE, base::Hours(kRefreshP3AFrequencyHours), this,
                      &BraveWalletP3A::OnUpdateTimerFired);
  OnUpdateTimerFired();  // Also call on startup
}

void BraveWalletP3A::Bind(
    mojo::PendingReceiver<mojom::BraveWalletP3A> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveWalletP3A::ReportUsage(bool unlocked) {
  VLOG(1) << "Wallet P3A: starting report";
  base::Time wallet_last_used =
      local_state_->GetTime(kBraveWalletLastUnlockTime);

  if (unlocked) {
    p3a_utils::RecordFeatureUsage(local_state_, kBraveWalletP3AFirstUnlockTime,
                                  kBraveWalletP3ALastUnlockTime);
    WriteUsageStatsToHistogram();
  } else {
    // Maybe record existing timestamp in case the user is not new.
    p3a_utils::MaybeRecordFeatureExistingUsageTimestamp(
        local_state_, kBraveWalletP3AFirstUnlockTime,
        kBraveWalletP3ALastUnlockTime, wallet_last_used);
  }

  p3a_utils::RecordFeatureNewUserReturning(
      local_state_, kBraveWalletP3AFirstUnlockTime,
      kBraveWalletP3ALastUnlockTime, kBraveWalletP3AUsedSecondDay,
      kBraveWalletNewUserReturningHistogramName);
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, kBraveWalletP3ALastUnlockTime,
      kBraveWalletLastUsageTimeHistogramName);

  ReportNftDiscoverySetting();
}

void BraveWalletP3A::ReportJSProvider(mojom::JSProviderType provider_type,
                                      mojom::CoinType coin_type,
                                      bool allow_provider_overwrite) {
  CHECK(coin_type == mojom::CoinType::ETH || coin_type == mojom::CoinType::SOL);

  const char* histogram_name;
  switch (coin_type) {
    case mojom::CoinType::ETH:
      histogram_name = kEthProviderHistogramName;
      break;
    case mojom::CoinType::SOL:
      histogram_name = kSolProviderHistogramName;
      break;
    default:
      return;
  }

  JSProviderAnswer answer = JSProviderAnswer::kNoWallet;
  bool is_wallet_setup = base::ranges::any_of(
      keyring_service_->GetAllAccountInfos(), [coin_type](auto& account) {
        return account->account_id->coin == coin_type;
      });

  switch (provider_type) {
    case mojom::JSProviderType::None:
      if (is_wallet_setup) {
        answer = JSProviderAnswer::kWalletDisabled;
      } else {
        answer = JSProviderAnswer::kNoWallet;
      }
      break;
    case mojom::JSProviderType::ThirdParty:
      // Third-party overriding is considered if the native wallet
      // is enabled and the native wallet is setup.
      answer = is_wallet_setup && allow_provider_overwrite
                   ? JSProviderAnswer::kThirdPartyOverriding
                   : JSProviderAnswer::kThirdPartyNotOverriding;
      break;
    case mojom::JSProviderType::Native:
      if (is_wallet_setup) {
        // A native wallet is definitely not being overridden
        // if provider overwrites are allowed.
        answer = !allow_provider_overwrite
                     ? JSProviderAnswer::kNativeOverridingDisallowed
                     : JSProviderAnswer::kNativeNotOverridden;
      }
      break;
  }

  base::UmaHistogramEnumeration(histogram_name, answer);
}

std::optional<mojom::OnboardingAction>
BraveWalletP3A::GetLastOnboardingAction() {
  if (local_state_->HasPrefPath(kBraveWalletP3AOnboardingLastStep)) {
    int pref_value =
        local_state_->GetInteger(kBraveWalletP3AOnboardingLastStep);
    return static_cast<mojom::OnboardingAction>(pref_value);
  }
  return std::nullopt;
}

void BraveWalletP3A::ReportOnboardingAction(mojom::OnboardingAction action) {
  if (action == mojom::OnboardingAction::StartRestore) {
    // We do not want to monitor wallet restores; cancel the
    // histogram record timer and wipe out the last onboarding step.
    local_state_->ClearPref(kBraveWalletP3AOnboardingLastStep);
    onboarding_report_timer_.Stop();
    return;
  }
  std::optional<mojom::OnboardingAction> last_step = GetLastOnboardingAction();
  if (!last_step.has_value() || *last_step < action) {
    // Only record steps that are ahead of the previous step so we
    // don't record back navigation.
    local_state_->SetInteger(kBraveWalletP3AOnboardingLastStep,
                             static_cast<int>(action));
  }
  if (onboarding_report_timer_.IsRunning() ||
      action == mojom::OnboardingAction::Shown) {
    // If the event is the first possible action (aka the shown event),
    // or if timer is already running (re)start the timer to debounce.
    onboarding_report_timer_.Start(
        FROM_HERE, kOnboardingRecordDelay,
        base::BindOnce(&BraveWalletP3A::RecordOnboardingHistogram,
                       base::Unretained(this)));
  } else {
    // If the timer is not running and the action is after the first possible
    // event, report it right away since it probably missed the 120 sec
    // deadline.
    RecordOnboardingHistogram();
  }
}

void BraveWalletP3A::RecordOnboardingHistogram() {
  std::optional<mojom::OnboardingAction> last_step = GetLastOnboardingAction();
  if (!last_step.has_value()) {
    return;
  }
  local_state_->ClearPref(kBraveWalletP3AOnboardingLastStep);
  UMA_HISTOGRAM_ENUMERATION(kOnboardingConversionHistogramName, *last_step);
}

void BraveWalletP3A::ReportTransactionSent(mojom::CoinType coin,
                                           bool new_send) {
  const char* histogram_name = nullptr;
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
    case mojom::CoinType::BTC:
      histogram_name = kBtcTransactionSentHistogramName;
      break;
    case mojom::CoinType::ZEC:
      histogram_name = kZecTransactionSentHistogramName;
      break;
    default:
      NOTREACHED() << coin;
  }

  CHECK(histogram_name);

  ScopedDictPrefUpdate last_sent_time_update(
      profile_prefs_, kBraveWalletLastTransactionSentTimeDict);
  base::Value::Dict& last_sent_time_dict = last_sent_time_update.Get();

  std::string coin_key = base::NumberToString(static_cast<int>(coin));

  base::Time now = base::Time::Now();
  base::Time last_sent_time = base::Time::FromSecondsSinceUnixEpoch(
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
    last_sent_time_dict.Set(coin_key, now.InSecondsFSinceUnixEpoch());
  }

  base::UmaHistogramExactLinear(histogram_name, answer, 2);
}

void BraveWalletP3A::RecordActiveWalletCount(int count,
                                             mojom::CoinType coin_type) {
  DCHECK_GE(count, 0);
  const char* histogram_name = nullptr;

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
    case mojom::CoinType::BTC:
      histogram_name = kBtcActiveAccountHistogramName;
      break;
    case mojom::CoinType::ZEC:
      histogram_name = kZecActiveAccountHistogramName;
      break;
    default:
      NOTREACHED() << coin_type;
  }

  CHECK(histogram_name);

  const base::Value::Dict& active_wallet_dict =
      profile_prefs_->GetDict(kBraveWalletP3AActiveWalletDict);
  std::string coin_type_str = base::NumberToString(static_cast<int>(coin_type));
  if (!active_wallet_dict.FindBool(coin_type_str).has_value()) {
    if (count == 0) {
      // Should not record zero to histogram if user never had an active
      // account, to avoid sending unnecessary data.
      return;
    }
    ScopedDictPrefUpdate active_wallet_dict_update(
        profile_prefs_, kBraveWalletP3AActiveWalletDict);
    active_wallet_dict_update->Set(coin_type_str, true);
  }
  p3a_utils::RecordToHistogramBucket(histogram_name, kActiveAccountBuckets,
                                     count);

  if (count > 0) {
    MaybeRecordNewUserBalance();
  }
}

void BraveWalletP3A::RecordNFTGalleryView(int nft_count) {
  if (!local_state_->GetBoolean(kBraveWalletP3ANFTGalleryUsed)) {
    local_state_->SetBoolean(kBraveWalletP3ANFTGalleryUsed, true);
    UMA_HISTOGRAM_BOOLEAN(kBraveWalletNFTNewUserHistogramName, true);
  }
  p3a_utils::RecordToHistogramBucket(kBraveWalletNFTCountHistogramName,
                                     kNFTCountBuckets, nft_count);
}

void BraveWalletP3A::MaybeRecordNewUserBalance() {
  base::Time deadline = base::Time::Now() - base::Days(7);
  if (local_state_->GetTime(kBraveWalletP3AFirstUnlockTime) >= deadline &&
      !local_state_->GetBoolean(kBraveWalletP3ANewUserBalanceReported)) {
    UMA_HISTOGRAM_BOOLEAN(kNewUserBalanceHistogramName, true);
    local_state_->SetBoolean(kBraveWalletP3ANewUserBalanceReported, true);
  }
}

void BraveWalletP3A::ReportNftDiscoverySetting() {
  if (!local_state_->GetTime(kBraveWalletLastUnlockTime).is_null()) {
    UMA_HISTOGRAM_BOOLEAN(
        kBraveWalletNFTDiscoveryEnabledHistogramName,
        profile_prefs_->GetBoolean(kBraveWalletNftDiscoveryEnabled));
  }
}

// TODO(djandries): remove pref migration around April 2024
void BraveWalletP3A::MigrateUsageProfilePrefsToLocalState() {
  for (const char* pref_name : kTimePrefsToMigrateToLocalState) {
    if (local_state_->GetTime(pref_name).is_null()) {
      base::Time profile_time = profile_prefs_->GetTime(pref_name);
      if (!profile_time.is_null()) {
        local_state_->SetTime(pref_name, profile_time);
        profile_prefs_->ClearPref(pref_name);
      }
    }
  }
  for (const char* pref_name : kTimePrefsToRemove) {
    local_state_->ClearPref(pref_name);
    profile_prefs_->ClearPref(pref_name);
  }
  if (!local_state_->GetBoolean(kBraveWalletP3AUsedSecondDay)) {
    bool profile_used_second_day =
        profile_prefs_->GetBoolean(kBraveWalletP3AUsedSecondDay);
    if (profile_used_second_day) {
      local_state_->SetBoolean(kBraveWalletP3AUsedSecondDay, true);
      profile_prefs_->ClearPref(kBraveWalletP3AUsedSecondDay);
    }
  }
  local_state_->ClearPref(kBraveWalletP3AWeeklyStorageDeprecated);
  profile_prefs_->ClearPref(kBraveWalletP3AWeeklyStorageDeprecated);
}

void BraveWalletP3A::OnUpdateTimerFired() {
  ReportUsage(false);
  for (const auto& coin : kAllCoins) {
    ReportTransactionSent(coin, false);
  }
}

void BraveWalletP3A::WriteUsageStatsToHistogram() {
  VLOG(1) << "Wallet P3A: Recording usage";
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletMonthlyHistogramName, true);
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletWeeklyHistogramName, true);
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletDailyHistogramName, true);
}

void BraveWalletP3A::RecordInitialBraveWalletP3AState() {
  RecordKeyringCreated(keyring_service_->IsWalletCreatedSync());
}

// KeyringServiceObserver
void BraveWalletP3A::WalletCreated() {
  RecordKeyringCreated(keyring_service_->IsWalletCreatedSync());
}

void BraveWalletP3A::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  if (tx_info->tx_status != mojom::TransactionStatus::Approved) {
    return;
  }

  auto tx_coin = GetCoinTypeFromTxDataUnion(*tx_info->tx_data_union);
  auto tx_type = tx_info->tx_type;
  auto count_test_networks = base::CommandLine::ForCurrentProcess()->HasSwitch(
      brave_wallet::mojom::kP3ACountTestNetworksSwitch);
  auto chain_id = tx_info->chain_id;

  if (tx_coin == mojom::CoinType::ETH) {
    if (tx_type != mojom::TransactionType::ETHSend &&
        tx_type != mojom::TransactionType::ERC20Transfer) {
      return;
    }
    if (!count_test_networks &&
        (chain_id == mojom::kSepoliaChainId ||
         chain_id == mojom::kLocalhostChainId ||
         chain_id == mojom::kFilecoinEthereumTestnetChainId)) {
      return;
    }
  } else if (tx_coin == mojom::CoinType::FIL) {
    if (tx_type != mojom::TransactionType::Other) {
      return;
    }
    if (!count_test_networks && (chain_id == mojom::kFilecoinTestnet ||
                                 chain_id == mojom::kLocalhostChainId)) {
      return;
    }
  } else if (tx_coin == mojom::CoinType::SOL) {
    if (tx_type != mojom::TransactionType::SolanaSystemTransfer &&
        tx_type != mojom::TransactionType::SolanaSPLTokenTransfer &&
        tx_type !=
            mojom::TransactionType::
                SolanaSPLTokenTransferWithAssociatedTokenAccountCreation) {
      return;
    }
    if (!count_test_networks && (chain_id == mojom::kSolanaTestnet ||
                                 chain_id == mojom::kSolanaDevnet ||
                                 chain_id == mojom::kLocalhostChainId)) {
      return;
    }
  } else if (tx_coin == mojom::CoinType::BTC) {
    if (tx_type != mojom::TransactionType::Other) {
      return;
    }
    if (!count_test_networks && chain_id == mojom::kBitcoinTestnet) {
      return;
    }
  } else if (tx_coin == mojom::CoinType::ZEC) {
    if (tx_type != mojom::TransactionType::Other) {
      return;
    }
    if (!count_test_networks && chain_id == mojom::kZCashTestnet) {
      return;
    }
  } else {
    NOTREACHED() << tx_coin;
  }
  ReportTransactionSent(tx_coin, true);
}

}  // namespace brave_wallet
