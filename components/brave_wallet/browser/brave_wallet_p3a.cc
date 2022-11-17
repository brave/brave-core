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
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
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
const char kBraveWalletWeeklyHistogramName[] = "Brave.Wallet.UsageDaysInWeek";
const char kBraveWalletMonthlyHistogramName[] = "Brave.Wallet.UsageMonthly.2";
const char kBraveWalletNewUserReturningHistogramName[] =
    "Brave.Wallet.NewUserReturning";
const char kBraveWalletLastUsageTimeHistogramName[] =
    "Brave.Wallet.LastUsageTime";

namespace {

constexpr int kRefreshP3AFrequencyHours = 24;
constexpr int kActiveAccountBuckets[] = {0, 1, 2, 3, 7};
const char* kTimePrefsToMigrateToLocalState[] = {
    kBraveWalletLastUnlockTime, kBraveWalletP3AFirstReportTime,
    kBraveWalletP3ALastReportTime, kBraveWalletP3AFirstUnlockTime,
    kBraveWalletP3ALastUnlockTime};

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
                               PrefService* profile_prefs,
                               PrefService* local_state)
    : wallet_service_(wallet_service),
      keyring_service_(keyring_service),
      profile_prefs_(profile_prefs),
      local_state_(local_state) {
  DCHECK(profile_prefs);
  DCHECK(local_state);

  MigrateUsageProfilePrefsToLocalState();

  RecordInitialBraveWalletP3AState();
  AddObservers();

  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(kBraveWalletLastUnlockTime,
                             base::BindRepeating(&BraveWalletP3A::ReportUsage,
                                                 base::Unretained(this), true));
}

BraveWalletP3A::~BraveWalletP3A() = default;

void BraveWalletP3A::AddObservers() {
  wallet_service_->AddObserver(
      wallet_service_observer_receiver_.BindNewPipeAndPassRemote());
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
  update_timer_.Start(FROM_HERE, base::Hours(kRefreshP3AFrequencyHours), this,
                      &BraveWalletP3A::OnUpdateTimerFired);
  OnUpdateTimerFired();  // Also call on startup
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

void BraveWalletP3A::ReportUsage(bool unlocked) {
  VLOG(1) << "Wallet P3A: starting report";
  base::Time wallet_last_used =
      local_state_->GetTime(kBraveWalletLastUnlockTime);
  base::Time first_p3a_report =
      local_state_->GetTime(kBraveWalletP3AFirstReportTime);
  base::Time last_p3a_report =
      local_state_->GetTime(kBraveWalletP3ALastReportTime);

  VLOG(1) << "Wallet P3A: first report: " << first_p3a_report
          << " last_report: " << last_p3a_report;

  WeeklyStorage weekly_store(local_state_, kBraveWalletP3AWeeklyStorage);
  if (wallet_last_used > last_p3a_report) {
    weekly_store.ReplaceTodaysValueIfGreater(1);
    VLOG(1) << "Wallet P3A: Reporting day in week, curr days in week val: "
            << weekly_store.GetWeeklySum();
  }

  WriteUsageStatsToHistogram(wallet_last_used, first_p3a_report,
                             last_p3a_report, weekly_store.GetWeeklySum());

  local_state_->SetTime(kBraveWalletP3ALastReportTime, base::Time::Now());
  if (first_p3a_report.is_null())
    local_state_->SetTime(kBraveWalletP3AFirstReportTime, base::Time::Now());

  if (unlocked) {
    p3a_utils::RecordFeatureUsage(local_state_, kBraveWalletP3AFirstUnlockTime,
                                  kBraveWalletP3ALastUnlockTime);
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
      profile_prefs_, kBraveWalletLastTransactionSentTimeDict);
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
      profile_prefs_->GetDict(kBraveWalletP3AActiveWalletDict);
  std::string coin_type_str = base::NumberToString(static_cast<int>(coin_type));
  if (!active_wallet_dict.FindBool(coin_type_str).has_value()) {
    if (count == 0) {
      // Should not record zero to histogram if user never had an active
      // account, to avoid sending unnecessary data.
      return;
    }
    DictionaryPrefUpdate active_wallet_dict_update(
        profile_prefs_, kBraveWalletP3AActiveWalletDict);
    active_wallet_dict_update->GetDict().Set(coin_type_str, true);
  }
  p3a_utils::RecordToHistogramBucket(histogram_name, kActiveAccountBuckets,
                                     count);
}

// TODO(djandries): remove pref migration after a few months have passed
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
  if (!local_state_->GetBoolean(kBraveWalletP3AUsedSecondDay)) {
    bool profile_used_second_day =
        profile_prefs_->GetBoolean(kBraveWalletP3AUsedSecondDay);
    if (profile_used_second_day) {
      local_state_->SetBoolean(kBraveWalletP3AUsedSecondDay, true);
      profile_prefs_->ClearPref(kBraveWalletP3AUsedSecondDay);
    }
  }
  if (local_state_->GetList(kBraveWalletP3AWeeklyStorage).empty()) {
    const base::Value::List& profile_weekly_list =
        profile_prefs_->GetList(kBraveWalletP3AWeeklyStorage);
    if (!profile_weekly_list.empty()) {
      local_state_->SetList(kBraveWalletP3AWeeklyStorage,
                            profile_weekly_list.Clone());
      profile_prefs_->ClearPref(kBraveWalletP3AWeeklyStorage);
    }
  }
}

void BraveWalletP3A::OnUpdateTimerFired() {
  ReportUsage(false);
  ReportTransactionSent(mojom::CoinType::ETH, false);
  ReportTransactionSent(mojom::CoinType::FIL, false);
  ReportTransactionSent(mojom::CoinType::SOL, false);
}

void BraveWalletP3A::WriteUsageStatsToHistogram(base::Time wallet_last_used,
                                                base::Time first_p3a_report,
                                                base::Time last_p3a_report,
                                                unsigned use_days_in_week) {
  base::Time::Exploded now_exp;
  base::Time::Exploded last_report_exp;
  base::Time::Exploded last_used_exp;
  base::Time::Now().LocalExplode(&now_exp);
  last_p3a_report.LocalExplode(&last_report_exp);
  wallet_last_used.LocalExplode(&last_used_exp);

  bool new_month_detected =
      !last_p3a_report.is_null() && (now_exp.year != last_report_exp.year ||
                                     now_exp.month != last_report_exp.month);

  if (new_month_detected) {
    bool used_last_month = !wallet_last_used.is_null() &&
                           last_report_exp.month == last_used_exp.month &&
                           last_report_exp.year == last_used_exp.year;
    VLOG(1) << "Wallet P3A: New month detected. used last month: "
            << used_last_month;
    UMA_HISTOGRAM_BOOLEAN(kBraveWalletMonthlyHistogramName, used_last_month);
  }

  bool week_passed_since_install =
      !first_p3a_report.is_null() &&
      (base::Time::Now() - first_p3a_report).InDays() >= 7;
  if (week_passed_since_install) {
    VLOG(1) << "Wallet P3A: recording daily/weekly. weekly_sum: "
            << use_days_in_week;
    UMA_HISTOGRAM_EXACT_LINEAR(kBraveWalletWeeklyHistogramName,
                               use_days_in_week, 8);
  } else {
    VLOG(1) << "Wallet P3A: Need 7 days of reports before recording "
               "daily/weekly, skipping";
  }
}

void BraveWalletP3A::RecordInitialBraveWalletP3AState() {
  keyring_service_->GetKeyringInfo(mojom::kDefaultKeyringId,
                                   base::BindOnce(&RecordKeyringCreated));
  RecordDefaultEthereumWalletSetting(profile_prefs_);
  RecordDefaultSolanaWalletSetting(profile_prefs_);
}

// KeyringServiceObserver
void BraveWalletP3A::KeyringCreated(const std::string& keyring_id) {
  keyring_service_->GetKeyringInfo(keyring_id,
                                   base::BindOnce(&RecordKeyringCreated));
}

// BraveWalletServiceObserver
void BraveWalletP3A::OnDefaultEthereumWalletChanged(
    brave_wallet::mojom::DefaultWallet default_wallet) {
  RecordDefaultEthereumWalletSetting(profile_prefs_);
}

// BraveWalletServiceObserver
void BraveWalletP3A::OnDefaultSolanaWalletChanged(
    brave_wallet::mojom::DefaultWallet default_wallet) {
  RecordDefaultSolanaWalletSetting(profile_prefs_);
}

}  // namespace brave_wallet
