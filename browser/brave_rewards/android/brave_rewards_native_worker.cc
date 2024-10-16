/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/android/brave_rewards_native_worker.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/build/android/jni_headers/BraveRewardsNativeWorker_jni.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/url_data_source.h"

#define DEFAULT_ADS_PER_HOUR 2

namespace chrome {
namespace android {

BraveRewardsNativeWorker::BraveRewardsNativeWorker(JNIEnv* env,
    const base::android::JavaRef<jobject>& obj):
    weak_java_brave_rewards_native_worker_(env, obj),
    weak_factory_(this) {
  Java_BraveRewardsNativeWorker_setNativePtr(env, obj,
    reinterpret_cast<intptr_t>(this));

  brave_rewards_service_ = brave_rewards::RewardsServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
  if (brave_rewards_service_) {
    rewards_service_observation_.Observe(brave_rewards_service_);

    if (auto* notification_service =
            brave_rewards_service_->GetNotificationService()) {
      rewards_notification_service_observation_.Observe(notification_service);
    }
  }
}

BraveRewardsNativeWorker::~BraveRewardsNativeWorker() {
}

void BraveRewardsNativeWorker::Destroy(JNIEnv* env) {
  delete this;
}

bool BraveRewardsNativeWorker::IsSupported(JNIEnv* env) {
  return brave_rewards::IsSupported(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile()->GetPrefs(),
      brave_rewards::IsSupportedOptions::kNone);
}

bool BraveRewardsNativeWorker::IsSupportedSkipRegionCheck(JNIEnv* env) {
  return brave_rewards::IsSupported(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile()->GetPrefs(),
      brave_rewards::IsSupportedOptions::kSkipRegionCheck);
}

std::string BraveRewardsNativeWorker::StringifyResult(
    brave_rewards::mojom::CreateRewardsWalletResult result) {
  switch (result) {
    case brave_rewards::mojom::CreateRewardsWalletResult::kSuccess:
      return "success";
    case brave_rewards::mojom::CreateRewardsWalletResult::
        kWalletGenerationDisabled:
      return "wallet-generation-disabled";
    case brave_rewards::mojom::CreateRewardsWalletResult::
        kGeoCountryAlreadyDeclared:
      return "country-already-declared";
    case brave_rewards::mojom::CreateRewardsWalletResult::kUnexpected:
      return "unexpected-error";
  }
}

bool BraveRewardsNativeWorker::IsRewardsEnabled(JNIEnv* env) {
  return ProfileManager::GetActiveUserProfile()
      ->GetOriginalProfile()
      ->GetPrefs()
      ->GetBoolean(brave_rewards::prefs::kEnabled);
}

bool BraveRewardsNativeWorker::ShouldShowSelfCustodyInvite(JNIEnv* env) {
  bool is_self_custody_invite_dismissed =
      ProfileManager::GetActiveUserProfile()
          ->GetOriginalProfile()
          ->GetPrefs()
          ->GetBoolean(brave_rewards::prefs::kSelfCustodyInviteDismissed);

  if (is_self_custody_invite_dismissed) {
    return false;
  }

  std::string country_code = brave_rewards_service_->GetCountryCode();
  const std::vector<std::string> providers =
      brave_rewards_service_->GetExternalWalletProviders();
  if (!base::Contains(providers,
                      brave_rewards::internal::constant::kWalletSolana)) {
    return false;
  }

  const auto& regions = parameters_->wallet_provider_regions.at(
      brave_rewards::internal::constant::kWalletSolana);
  if (!regions) {
    return true;
  }

  const auto& [allow, block] = *regions;
  if (allow.empty() && block.empty()) {
    return true;
  }

  return base::Contains(allow, country_code) ||
         (!block.empty() && !base::Contains(block, country_code));
}

void BraveRewardsNativeWorker::CreateRewardsWallet(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& country_code) {
  if (brave_rewards_service_) {
    brave_rewards_service_->CreateRewardsWallet(
        base::android::ConvertJavaStringToUTF8(env, country_code),
        base::BindOnce(&BraveRewardsNativeWorker::OnCreateRewardsWallet,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnCreateRewardsWallet(
    brave_rewards::mojom::CreateRewardsWalletResult result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onCreateRewardsWallet(
      env, weak_java_brave_rewards_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, StringifyResult(result)));
}

void BraveRewardsNativeWorker::GetRewardsParameters(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetRewardsParameters(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetRewardsParameters,
                       weak_factory_.GetWeakPtr(), brave_rewards_service_));
  }
}

void BraveRewardsNativeWorker::OnGetRewardsParameters(
    brave_rewards::RewardsService* rewards_service,
    brave_rewards::mojom::RewardsParametersPtr parameters) {
  if (parameters) {
    parameters_ = std::move(parameters);
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onRewardsParameters(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

void BraveRewardsNativeWorker::OnTermsOfServiceUpdateAccepted() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onTermsOfServiceUpdateAccepted(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

double BraveRewardsNativeWorker::GetVbatDeadline(JNIEnv* env) {
  if (parameters_) {
    if (!parameters_->vbat_deadline.is_null()) {
      return floor(parameters_->vbat_deadline.InSecondsFSinceUnixEpoch() *
                   base::Time::kMillisecondsPerSecond);
    }
  }
  return 0.0;
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetPayoutStatus(JNIEnv* env) {
  std::string wallet_type;
  std::string payout_status;
  if (brave_rewards_service_) {
    wallet_type = brave_rewards_service_->GetExternalWalletType();
    if (parameters_) {
      if (!parameters_->payout_status.empty()) {
        payout_status = parameters_->payout_status.at(wallet_type);
      }
    }
  }
  return base::android::ConvertUTF8ToJavaString(env, payout_status);
}

void BraveRewardsNativeWorker::GetUserType(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetUserType(base::BindOnce(
        &BraveRewardsNativeWorker::OnGetUserType, weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetUserType(
    const brave_rewards::mojom::UserType user_type) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onGetUserType(
      env, weak_java_brave_rewards_native_worker_.get(env),
      static_cast<int>(user_type));
}

void BraveRewardsNativeWorker::FetchBalance(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->FetchBalance(base::BindOnce(
        &BraveRewardsNativeWorker::OnBalance, weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnBalance(
    brave_rewards::mojom::BalancePtr balance) {
  if (balance) {
    balance_ = *balance;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onBalance(
      env, weak_java_brave_rewards_native_worker_.get(env), !!balance);
}

void BraveRewardsNativeWorker::GetPublisherInfo(
    JNIEnv* env,
    int tabId,
    const base::android::JavaParamRef<jstring>& host) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetPublisherActivityFromUrl(
        tabId, base::android::ConvertJavaStringToUTF8(env, host), "", "");
  }
}

void BraveRewardsNativeWorker::OnPanelPublisherInfo(
    brave_rewards::RewardsService* rewards_service,
    const brave_rewards::mojom::Result result,
    const brave_rewards::mojom::PublisherInfo* info,
    uint64_t tabId) {
  if (!info) {
    return;
  }
  brave_rewards::mojom::PublisherInfoPtr pi = info->Clone();
  map_publishers_info_[tabId] = std::move(pi);
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onPublisherInfo(
      env, weak_java_brave_rewards_native_worker_.get(env), tabId);
}

void BraveRewardsNativeWorker::OnReconcileComplete(
    brave_rewards::RewardsService* rewards_service,
    const brave_rewards::mojom::Result result,
    const std::string& contribution_id,
    const double amount,
    const brave_rewards::mojom::RewardsType type,
    const brave_rewards::mojom::ContributionProcessor processor) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onReconcileComplete(
      env, weak_java_brave_rewards_native_worker_.get(env),
      static_cast<int>(result), static_cast<int>(type), amount);
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetPublisherURL(JNIEnv* env, uint64_t tabId) {
  base::android::ScopedJavaLocalRef<jstring> res =
    base::android::ConvertUTF8ToJavaString(env, "");

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = base::android::ConvertUTF8ToJavaString(env, iter->second->url);
  }

  return res;
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetPublisherFavIconURL(JNIEnv* env, uint64_t tabId) {
  base::android::ScopedJavaLocalRef<jstring> res =
    base::android::ConvertUTF8ToJavaString(env, "");

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = base::android::ConvertUTF8ToJavaString(env,
      iter->second->favicon_url);
  }

  return res;
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetCaptchaSolutionURL(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& paymentId,
    const base::android::JavaParamRef<jstring>& captchaId) {
  const std::string path = base::StringPrintf(
      "/v3/captcha/solution/%s/%s",
      base::android::ConvertJavaStringToUTF8(env, paymentId).c_str(),
      base::android::ConvertJavaStringToUTF8(env, captchaId).c_str());
  std::string captcha_solution_url =
      brave_adaptive_captcha::ServerUtil::GetInstance()->GetServerUrl(path);

  return base::android::ConvertUTF8ToJavaString(env, captcha_solution_url);
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetAttestationURL(JNIEnv* env) {
  const std::string path = "/v1/attestations/android";
  std::string captcha_solution_url =
      brave_adaptive_captcha::ServerUtil::GetInstance()->GetServerUrl(path);

  return base::android::ConvertUTF8ToJavaString(env, captcha_solution_url);
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetAttestationURLWithPaymentId(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& paymentId) {
  const std::string path = base::StringPrintf(
      "/v1/attestations/android/%s",
      base::android::ConvertJavaStringToUTF8(env, paymentId).c_str());
  std::string captcha_solution_url =
      brave_adaptive_captcha::ServerUtil::GetInstance()->GetServerUrl(path);

  return base::android::ConvertUTF8ToJavaString(env, captcha_solution_url);
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetPublisherName(JNIEnv* env, uint64_t tabId) {
  base::android::ScopedJavaLocalRef<jstring> res =
    base::android::ConvertUTF8ToJavaString(env, "");

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = base::android::ConvertUTF8ToJavaString(env, iter->second->name);
  }

  return res;
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetPublisherId(JNIEnv* env, uint64_t tabId) {
  base::android::ScopedJavaLocalRef<jstring> res =
    base::android::ConvertUTF8ToJavaString(env, "");

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = base::android::ConvertUTF8ToJavaString(env, iter->second->id);
  }

  return res;
}

int BraveRewardsNativeWorker::GetPublisherPercent(JNIEnv* env, uint64_t tabId) {
  int res = 0;

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = iter->second->percent;
  }

  return res;
}

bool BraveRewardsNativeWorker::GetPublisherExcluded(JNIEnv* env,
                                                    uint64_t tabId) {
  bool res = false;

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = iter->second->excluded ==
          brave_rewards::mojom::PublisherExclude::EXCLUDED;
  }

  return res;
}

int BraveRewardsNativeWorker::GetPublisherStatus(JNIEnv* env, uint64_t tabId) {
  int res =
      static_cast<int>(brave_rewards::mojom::PublisherStatus::NOT_VERIFIED);
  PublishersInfoMap::const_iterator iter = map_publishers_info_.find(tabId);
  if (iter != map_publishers_info_.end()) {
    res = static_cast<int>(iter->second->status);
  }
  return res;
}

void BraveRewardsNativeWorker::IncludeInAutoContribution(JNIEnv* env,
                                                         uint64_t tabId,
                                                         bool exclude) {
  PublishersInfoMap::iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    if (exclude) {
      iter->second->excluded = brave_rewards::mojom::PublisherExclude::EXCLUDED;
    } else {
      iter->second->excluded = brave_rewards::mojom::PublisherExclude::INCLUDED;
    }
    if (brave_rewards_service_) {
      brave_rewards_service_->SetPublisherExclude(iter->second->id, exclude);
    }
  }
}

void BraveRewardsNativeWorker::RemovePublisherFromMap(JNIEnv* env,
                                                      uint64_t tabId) {
  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    map_publishers_info_.erase(iter);
  }
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetWalletBalance(JNIEnv* env) {
  std::string json_balance;
  base::Value::Dict root;
  root.Set("total", balance_.total);

  base::Value::Dict json_wallets;
  for (const auto & item : balance_.wallets) {
    json_wallets.Set(item.first, item.second);
  }
  root.SetByDottedPath("wallets", std::move(json_wallets));
  base::JSONWriter::Write(std::move(root), &json_balance);

  return base::android::ConvertUTF8ToJavaString(env, json_balance);
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetExternalWalletType(JNIEnv* env) {
  std::string wallet_type;
  if (brave_rewards_service_) {
    wallet_type = brave_rewards_service_->GetExternalWalletType();
  }

  return base::android::ConvertUTF8ToJavaString(env, wallet_type);
}

void BraveRewardsNativeWorker::GetAdsAccountStatement(JNIEnv* env) {
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
  if (!ads_service) {
    return;
  }
  ads_service->GetStatementOfAccounts(
      base::BindOnce(&BraveRewardsNativeWorker::OnGetAdsAccountStatement,
                     weak_factory_.GetWeakPtr()));
}

void BraveRewardsNativeWorker::OnGetAdsAccountStatement(
    brave_ads::mojom::StatementInfoPtr statement) {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!statement) {
    Java_BraveRewardsNativeWorker_onGetAdsAccountStatement(
        env, weak_java_brave_rewards_native_worker_.get(env),
        /* success */ false, 0.0, 0, 0.0, 0.0, 0.0, 0.0);
    return;
  }

  Java_BraveRewardsNativeWorker_onGetAdsAccountStatement(
      env, weak_java_brave_rewards_native_worker_.get(env),
      /* success */ true,
      statement->next_payment_date.InSecondsFSinceUnixEpoch() * 1000,
      statement->ads_received_this_month, statement->min_earnings_this_month,
      statement->max_earnings_this_month,
      statement->min_earnings_previous_month,
      statement->max_earnings_previous_month);
}

bool BraveRewardsNativeWorker::CanConnectAccount(JNIEnv* env) {
  if (!parameters_ || !brave_rewards_service_) {
    return true;
  }
  std::string country_code = brave_rewards_service_->GetCountryCode();
  return base::ranges::any_of(
      brave_rewards_service_->GetExternalWalletProviders(),
      [this, &country_code](const std::string& provider) {
        if (!parameters_->wallet_provider_regions.count(provider)) {
          return true;
        }

        const auto& regions = parameters_->wallet_provider_regions.at(provider);
        if (!regions) {
          return true;
        }

        const auto& [allow, block] = *regions;
        if (allow.empty() && block.empty()) {
          return true;
        }

        return base::Contains(allow, country_code) ||
               (!block.empty() && !base::Contains(block, country_code));
      });
}

base::android::ScopedJavaLocalRef<jdoubleArray>
BraveRewardsNativeWorker::GetTipChoices(JNIEnv* env) {
  return base::android::ToJavaDoubleArray(
      env, parameters_ ? parameters_->tip_choices : std::vector<double>());
}

double BraveRewardsNativeWorker::GetWalletRate(JNIEnv* env) {
  return parameters_ ? parameters_->rate : 0.0;
}

void BraveRewardsNativeWorker::GetCurrentBalanceReport(JNIEnv* env) {
  if (brave_rewards_service_) {
    auto now = base::Time::Now();
    base::Time::Exploded exploded;
    now.LocalExplode(&exploded);

    brave_rewards_service_->GetBalanceReport(
        exploded.month, exploded.year,
        base::BindOnce(&BraveRewardsNativeWorker::OnGetCurrentBalanceReport,
                       weak_factory_.GetWeakPtr(), brave_rewards_service_));
  }
}

void BraveRewardsNativeWorker::OnGetCurrentBalanceReport(
    brave_rewards::RewardsService* rewards_service,
    const brave_rewards::mojom::Result result,
    brave_rewards::mojom::BalanceReportInfoPtr report) {
  base::android::ScopedJavaLocalRef<jdoubleArray> java_array;
  JNIEnv* env = base::android::AttachCurrentThread();
  if (report) {
    std::vector<double> values;
    values.push_back(report->earning_from_ads);
    values.push_back(report->auto_contribute);
    values.push_back(report->recurring_donation);
    values.push_back(report->one_time_donation);
    java_array = base::android::ToJavaDoubleArray(env, values);
  }
  Java_BraveRewardsNativeWorker_onGetCurrentBalanceReport(
      env, weak_java_brave_rewards_native_worker_.get(env), java_array);
}

void BraveRewardsNativeWorker::Donate(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& publisher_key,
    double amount,
    bool recurring) {
  if (brave_rewards_service_) {
    brave_rewards_service_->SendContribution(
        base::android::ConvertJavaStringToUTF8(env, publisher_key), amount,
        recurring,
        base::BindOnce(&BraveRewardsNativeWorker::OnSendContribution,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnSendContribution(bool result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onSendContribution(
      env, weak_java_brave_rewards_native_worker_.get(env), result);
}

void BraveRewardsNativeWorker::GetAllNotifications(JNIEnv* env) {
  if (rewards_notification_service_observation_.IsObserving()) {
    rewards_notification_service_observation_.GetSource()->GetNotifications();
  }
}

void BraveRewardsNativeWorker::DeleteNotification(JNIEnv* env,
        const base::android::JavaParamRef<jstring>& notification_id) {
  if (rewards_notification_service_observation_.IsObserving()) {
    rewards_notification_service_observation_.GetSource()->DeleteNotification(
        base::android::ConvertJavaStringToUTF8(env, notification_id));
  }
}

void BraveRewardsNativeWorker::GetRecurringDonations(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetRecurringTips(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetRecurringTips,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetRecurringTips(
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
  map_recurrent_publishers_.clear();
  for (const auto& item : list) {
    map_recurrent_publishers_[item->id] = item->Clone();
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onRecurringDonationUpdated(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

bool BraveRewardsNativeWorker::IsCurrentPublisherInRecurrentDonations(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& publisher) {
  return map_recurrent_publishers_.find(
    base::android::ConvertJavaStringToUTF8(env, publisher)) !=
      map_recurrent_publishers_.end();
}

void BraveRewardsNativeWorker::GetAutoContributeProperties(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetAutoContributeProperties(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetAutoContributeProperties,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetAutoContributeProperties(
    brave_rewards::mojom::AutoContributePropertiesPtr properties) {
  if (properties) {
    auto_contrib_properties_ = std::move(properties);
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onGetAutoContributeProperties(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

bool BraveRewardsNativeWorker::IsAutoContributeEnabled(JNIEnv* env) {
  if (!auto_contrib_properties_) {
    return false;
  }

  return auto_contrib_properties_->enabled_contribute;
}

void BraveRewardsNativeWorker::GetReconcileStamp(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetReconcileStamp(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetGetReconcileStamp,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::ResetTheWholeState(JNIEnv* env) {
  if (!brave_rewards_service_) {
    OnResetTheWholeState(false);
    return;
  }
  brave_rewards_service_->CompleteReset(
      base::BindOnce(&BraveRewardsNativeWorker::OnResetTheWholeState,
                     weak_factory_.GetWeakPtr()));
}

void BraveRewardsNativeWorker::OnCompleteReset(const bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_onCompleteReset(
      env, weak_java_brave_rewards_native_worker_.get(env), success);
}

void BraveRewardsNativeWorker::OnResetTheWholeState(const bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_onResetTheWholeState(
      env, weak_java_brave_rewards_native_worker_.get(env), success);
}

double BraveRewardsNativeWorker::GetPublisherRecurrentDonationAmount(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& publisher) {
  double amount(0.0);
  auto it = map_recurrent_publishers_.find(
    base::android::ConvertJavaStringToUTF8(env, publisher));

  if (it != map_recurrent_publishers_.end()) {
    amount = it->second->weight;
  }
  return  amount;
}

void BraveRewardsNativeWorker::RemoveRecurring(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& publisher) {
  if (brave_rewards_service_) {
    brave_rewards_service_->RemoveRecurringTip(
        base::android::ConvertJavaStringToUTF8(env, publisher));
    auto it = map_recurrent_publishers_.find(
        base::android::ConvertJavaStringToUTF8(env, publisher));

    if (it != map_recurrent_publishers_.end()) {
      map_recurrent_publishers_.erase(it);
    }
  }
}

void BraveRewardsNativeWorker::OnGetGetReconcileStamp(uint64_t timestamp) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_onGetReconcileStamp(
      env, weak_java_brave_rewards_native_worker_.get(env), timestamp);
}

void BraveRewardsNativeWorker::OnNotificationAdded(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_onNotificationAdded(
      env, weak_java_brave_rewards_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, notification.id_),
      notification.type_, notification.timestamp_,
      base::android::ToJavaArrayOfStrings(env, notification.args_));
}

void BraveRewardsNativeWorker::OnGetAllNotifications(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotificationsList&
      notifications_list) {
  JNIEnv* env = base::android::AttachCurrentThread();

  // Notify about notifications count
  Java_BraveRewardsNativeWorker_onNotificationsCount(
      env, weak_java_brave_rewards_native_worker_.get(env),
      notifications_list.size());

  brave_rewards::RewardsNotificationService::RewardsNotificationsList::
    const_iterator iter =
      std::max_element(notifications_list.begin(), notifications_list.end(),
        [](const brave_rewards::RewardsNotificationService::
            RewardsNotification& notification_a,
          const brave_rewards::RewardsNotificationService::
            RewardsNotification& notification_b) {
        return notification_a.timestamp_ > notification_b.timestamp_;
      });

  if (iter != notifications_list.end()) {
    Java_BraveRewardsNativeWorker_onGetLatestNotification(
        env, weak_java_brave_rewards_native_worker_.get(env),
        base::android::ConvertUTF8ToJavaString(env, iter->id_), iter->type_,
        iter->timestamp_,
        base::android::ToJavaArrayOfStrings(env, iter->args_));
  }
}

void BraveRewardsNativeWorker::OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_onNotificationDeleted(
      env, weak_java_brave_rewards_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, notification.id_));
}

int BraveRewardsNativeWorker::GetAdsPerHour(JNIEnv* env) {
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
  if (!ads_service_) {
    return DEFAULT_ADS_PER_HOUR;
  }
  return ads_service_->GetMaximumNotificationAdsPerHour();
}

void BraveRewardsNativeWorker::SetAdsPerHour(JNIEnv* env, jint value) {
  ProfileManager::GetActiveUserProfile()
      ->GetOriginalProfile()
      ->GetPrefs()
      ->SetInt64(brave_ads::prefs::kMaximumNotificationAdsPerHour,
                 static_cast<int64_t>(value));
}

void BraveRewardsNativeWorker::SetAutoContributionAmount(JNIEnv* env,
                                                         jdouble value) {
  if (brave_rewards_service_) {
    brave_rewards_service_->SetAutoContributionAmount(value);
  }
}

void BraveRewardsNativeWorker::GetAutoContributionAmount(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetAutoContributionAmount(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetAutoContributionAmount,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetAutoContributionAmount(
    double auto_contribution_amount) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onGetAutoContributionAmount(
      env, weak_java_brave_rewards_native_worker_.get(env),
      auto_contribution_amount);
}

void BraveRewardsNativeWorker::GetExternalWallet(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetExternalWallet(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetExternalWallet,
                       weak_factory_.GetWeakPtr()));
  }
}

bool BraveRewardsNativeWorker::IsTermsOfServiceUpdateRequired(JNIEnv* env) {
  if (!brave_rewards_service_) {
    return false;
  }
  return brave_rewards_service_->IsTermsOfServiceUpdateRequired();
}

void BraveRewardsNativeWorker::AcceptTermsOfServiceUpdate(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->AcceptTermsOfServiceUpdate();
  }
}

base::android::ScopedJavaLocalRef<jstring>
BraveRewardsNativeWorker::GetCountryCode(JNIEnv* env) {
  std::string country_code;
  if (brave_rewards_service_) {
    country_code = brave_rewards_service_->GetCountryCode();
  }

  return base::android::ConvertUTF8ToJavaString(env, country_code);
}

void BraveRewardsNativeWorker::GetAvailableCountries(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetAvailableCountries(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetAvailableCountries,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetAvailableCountries(
    std::vector<std::string> countries) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onGetAvailableCountries(
      env, weak_java_brave_rewards_native_worker_.get(env),
      base::android::ToJavaArrayOfStrings(env, countries));
}

void BraveRewardsNativeWorker::GetPublishersVisitedCount(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetPublishersVisitedCount(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetPublishersVisitedCount,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetPublishersVisitedCount(int count) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onGetPublishersVisitedCount(
      env, weak_java_brave_rewards_native_worker_.get(env), count);
}

void BraveRewardsNativeWorker::GetPublisherBanner(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& publisher_key) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetPublisherBanner(
        base::android::ConvertJavaStringToUTF8(env, publisher_key),
        base::BindOnce(&BraveRewardsNativeWorker::onPublisherBanner,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::onPublisherBanner(
    brave_rewards::mojom::PublisherBannerPtr banner) {
  std::string json_banner_info;
  if (!banner) {
    json_banner_info = "";
  } else {
    base::Value::Dict dict;
    dict.Set("publisher_key", banner->publisher_key);
    dict.Set("title", banner->title);

    dict.Set("name", banner->name);
    dict.Set("description", banner->description);
    dict.Set("background", banner->background);
    dict.Set("logo", banner->logo);
    dict.Set("provider", banner->provider);
    dict.Set("web3_url", banner->web3_url);

    base::Value::Dict links;
    for (auto const& link : banner->links) {
      links.Set(link.first, link.second);
    }
    dict.Set("links", std::move(links));

    dict.Set("status", static_cast<int32_t>(banner->status));
    base::JSONWriter::Write(dict, &json_banner_info);
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onPublisherBanner(
      env, weak_java_brave_rewards_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_banner_info));
}

void BraveRewardsNativeWorker::OnGetExternalWallet(
    brave_rewards::mojom::ExternalWalletPtr wallet) {
  std::string json_wallet;
  if (!wallet) {
    // If the user does not have an external wallet, expose a default/empty
    // wallet for backward compatibility with Android code that expects an
    // external wallet structure with a NOT_CONNECTED status.
    base::Value::Dict dict;
    dict.Set("token", "");
    dict.Set("address", "");
    dict.Set("status", static_cast<int32_t>(
                           brave_rewards::mojom::WalletStatus::kNotConnected));
    dict.Set("type", brave_rewards_service_->GetExternalWalletType());
    dict.Set("user_name", "");
    dict.Set("account_url", "");
    base::JSONWriter::Write(dict, &json_wallet);
  } else {
    base::Value::Dict dict;
    dict.Set("token", wallet->token);
    dict.Set("address", wallet->address);

    // enum class WalletStatus : int32_t
    dict.Set("status", static_cast<int32_t>(wallet->status));
    dict.Set("type", wallet->type);
    dict.Set("user_name", wallet->user_name);
    dict.Set("account_url", wallet->account_url);
    base::JSONWriter::Write(dict, &json_wallet);
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onGetExternalWallet(
      env, weak_java_brave_rewards_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_wallet));
}

void BraveRewardsNativeWorker::DisconnectWallet(JNIEnv* env) {
  // TODO(zenparsing): Remove disconnect ability from Android UI.
}

void BraveRewardsNativeWorker::OnExternalWalletConnected() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onExternalWalletConnected(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

void BraveRewardsNativeWorker::OnExternalWalletLoggedOut() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onExternalWalletLoggedOut(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

void BraveRewardsNativeWorker::OnExternalWalletReconnected() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onExternalWalletReconnected(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

std::string BraveRewardsNativeWorker::StdStrStrMapToJsonString(
    const base::flat_map<std::string, std::string>& args) {
    std::string json_args;
    base::Value::Dict dict;
    for (const auto & item : args) {
      dict.Set(item.first, item.second);
    }
    base::JSONWriter::Write(dict, &json_args);
    return json_args;
}

void BraveRewardsNativeWorker::RefreshPublisher(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& publisher_key) {
  if (!brave_rewards_service_) {
    return;
  }
  brave_rewards_service_->RefreshPublisher(
      base::android::ConvertJavaStringToUTF8(env, publisher_key),
      base::BindOnce(&BraveRewardsNativeWorker::OnRefreshPublisher,
                     weak_factory_.GetWeakPtr()));
}

void BraveRewardsNativeWorker::OnRefreshPublisher(
    const brave_rewards::mojom::PublisherStatus status,
    const std::string& publisher_key) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_onRefreshPublisher(
      env, weak_java_brave_rewards_native_worker_.get(env),
      static_cast<int>(status),
      base::android::ConvertUTF8ToJavaString(env, publisher_key));
}

void BraveRewardsNativeWorker::SetAutoContributeEnabled(
    JNIEnv* env,
    bool isAutoContributeEnabled) {
  if (brave_rewards_service_) {
    brave_rewards_service_->SetAutoContributeEnabled(isAutoContributeEnabled);
  }
}

void BraveRewardsNativeWorker::RecordPanelTrigger(JNIEnv* env) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetP3AConversionMonitor()->RecordPanelTrigger(
        brave_rewards::p3a::PanelTrigger::kToolbarButton);
  }
}

static void JNI_BraveRewardsNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveRewardsNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
