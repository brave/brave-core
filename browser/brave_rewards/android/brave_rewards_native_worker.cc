/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/android/brave_rewards_native_worker.h"

#include <iomanip>
#include <string>
#include <vector>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/jni_array.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/url_data_source.h"
#include "brave/build/android/jni_headers/BraveRewardsNativeWorker_jni.h"

#define DEFAULT_ADS_PER_HOUR 2
#define DEFAULT_AUTO_CONTRIBUTION_AMOUNT 10

namespace chrome {
namespace android {

BraveRewardsNativeWorker::BraveRewardsNativeWorker(JNIEnv* env,
    const base::android::JavaRef<jobject>& obj):
    weak_java_brave_rewards_native_worker_(env, obj),
    brave_rewards_service_(nullptr),
    weak_factory_(this) {
  Java_BraveRewardsNativeWorker_setNativePtr(env, obj,
    reinterpret_cast<intptr_t>(this));

  brave_rewards_service_ = brave_rewards::RewardsServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
  if (brave_rewards_service_) {
    brave_rewards_service_->AddObserver(this);
    brave_rewards_service_->AddPrivateObserver(this);
    brave_rewards::RewardsNotificationService* notification_service =
      brave_rewards_service_->GetNotificationService();
    if (notification_service) {
      notification_service->AddObserver(this);
    }
  }
}

BraveRewardsNativeWorker::~BraveRewardsNativeWorker() {
}

void BraveRewardsNativeWorker::Destroy(JNIEnv* env, const
        base::android::JavaParamRef<jobject>& jcaller) {
  if (brave_rewards_service_) {
    brave_rewards_service_->RemoveObserver(this);
    brave_rewards_service_->RemovePrivateObserver(this);
    brave_rewards::RewardsNotificationService* notification_service =
      brave_rewards_service_->GetNotificationService();
    if (notification_service) {
      notification_service->RemoveObserver(this);
    }
  }
  delete this;
}

void BraveRewardsNativeWorker::GetRewardsParameters(JNIEnv* env, const
        base::android::JavaParamRef<jobject>& jcaller) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetRewardsParameters(
        base::BindOnce(&BraveRewardsNativeWorker::OnGetRewardsParameters,
                       base::Unretained(this), brave_rewards_service_));
  }
}

void BraveRewardsNativeWorker::GetPublisherInfo(JNIEnv* env, const
        base::android::JavaParamRef<jobject>& jcaller, int tabId,
        const base::android::JavaParamRef<jstring>& host) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetPublisherActivityFromUrl(tabId,
      base::android::ConvertJavaStringToUTF8(env, host), "", "");
  }
}

void BraveRewardsNativeWorker::OnPanelPublisherInfo(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const ledger::type::PublisherInfo* info,
      uint64_t tabId) {
  if (!info) {
    return;
  }
  ledger::type::PublisherInfoPtr pi = info->Clone();
  map_publishers_info_[tabId] = std::move(pi);
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnPublisherInfo(env,
        weak_java_brave_rewards_native_worker_.get(env), tabId);
}

base::android::ScopedJavaLocalRef<jstring>
  BraveRewardsNativeWorker::GetPublisherURL(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  base::android::ScopedJavaLocalRef<jstring> res =
    base::android::ConvertUTF8ToJavaString(env, "");

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = base::android::ConvertUTF8ToJavaString(env, iter->second->url);
  }

  return res;
}

base::android::ScopedJavaLocalRef<jstring>
  BraveRewardsNativeWorker::GetPublisherFavIconURL(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
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
  BraveRewardsNativeWorker::GetPublisherName(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  base::android::ScopedJavaLocalRef<jstring> res =
    base::android::ConvertUTF8ToJavaString(env, "");

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = base::android::ConvertUTF8ToJavaString(env, iter->second->name);
  }

  return res;
}

base::android::ScopedJavaLocalRef<jstring>
  BraveRewardsNativeWorker::GetPublisherId(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  base::android::ScopedJavaLocalRef<jstring> res =
    base::android::ConvertUTF8ToJavaString(env, "");

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = base::android::ConvertUTF8ToJavaString(env, iter->second->id);
  }

  return res;
}

int BraveRewardsNativeWorker::GetPublisherPercent(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  int res = 0;

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = iter->second->percent;
  }

  return res;
}

bool BraveRewardsNativeWorker::GetPublisherExcluded(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  bool res = false;

  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    res = iter->second->excluded == ledger::type::PublisherExclude::EXCLUDED;
  }

  return res;
}

int BraveRewardsNativeWorker::GetPublisherStatus(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  int res = static_cast<int>(ledger::type::PublisherStatus::NOT_VERIFIED);
  PublishersInfoMap::const_iterator iter = map_publishers_info_.find(tabId);
  if (iter != map_publishers_info_.end()) {
    res = static_cast<int>(iter->second->status);
  }
  return res;
}

void BraveRewardsNativeWorker::IncludeInAutoContribution(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId,
        bool exclude) {
  PublishersInfoMap::iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    if (exclude) {
      iter->second->excluded = ledger::type::PublisherExclude::EXCLUDED;
    } else {
      iter->second->excluded = ledger::type::PublisherExclude::INCLUDED;
    }
    if (brave_rewards_service_) {
      brave_rewards_service_->SetPublisherExclude(iter->second->id, exclude);
    }
  }
}

void BraveRewardsNativeWorker::RemovePublisherFromMap(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  PublishersInfoMap::const_iterator iter(map_publishers_info_.find(tabId));
  if (iter != map_publishers_info_.end()) {
    map_publishers_info_.erase(iter);
  }
}

void BraveRewardsNativeWorker::OnGetRewardsParameters(
    brave_rewards::RewardsService* rewards_service,
    ledger::type::RewardsParametersPtr parameters) {
  if (parameters) {
    parameters_ = *parameters;
  }

  if (rewards_service) {
    rewards_service->FetchBalance(
      base::Bind(
        &BraveRewardsNativeWorker::OnBalance,
        weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnBalance(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance) {
  if (result == ledger::type::Result::LEDGER_OK && balance) {
    balance_ = *balance;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnRewardsParameters(
      env, weak_java_brave_rewards_native_worker_.get(env), 0);
}

base::android::ScopedJavaLocalRef<jstring>
    BraveRewardsNativeWorker::GetWalletBalance(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  std::string json_balance;
  base::DictionaryValue json_root;
  json_root.SetDoubleKey("total", balance_.total);

  auto json_wallets = std::make_unique<base::DictionaryValue>();
  for (const auto & item : balance_.wallets) {
    json_wallets->SetDoubleKey(item.first, item.second);
  }
  json_root.SetDictionary("wallets", std::move(json_wallets));
  base::JSONWriter::Write(json_root, &json_balance);

  return base::android::ConvertUTF8ToJavaString(env, json_balance);
}

double BraveRewardsNativeWorker::GetWalletRate(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  return parameters_.rate;
}

void BraveRewardsNativeWorker::FetchGrants(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->FetchPromotions();
  }
}

void BraveRewardsNativeWorker::StartProcess(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->StartProcess(base::Bind(
          &BraveRewardsNativeWorker::OnStartProcess,
          weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnStartProcess(
    const ledger::type::Result result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnStartProcess(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

void BraveRewardsNativeWorker::GetCurrentBalanceReport(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    auto now = base::Time::Now();
    base::Time::Exploded exploded;
    now.LocalExplode(&exploded);

    brave_rewards_service_->GetBalanceReport(
        exploded.month, exploded.year,
        base::BindOnce(&BraveRewardsNativeWorker::OnGetCurrentBalanceReport,
                       base::Unretained(this), brave_rewards_service_));
  }
}

void BraveRewardsNativeWorker::OnGetCurrentBalanceReport(
        brave_rewards::RewardsService* rewards_service,
        const ledger::type::Result result,
        ledger::type::BalanceReportInfoPtr report) {
  base::android::ScopedJavaLocalRef<jdoubleArray> java_array;
  JNIEnv* env = base::android::AttachCurrentThread();
  if (report) {
    std::vector<double> values;
    values.push_back(report->grants);
    values.push_back(report->earning_from_ads);
    values.push_back(report->auto_contribute);
    values.push_back(report->recurring_donation);
    values.push_back(report->one_time_donation);
    java_array = base::android::ToJavaDoubleArray(env, values);
  }
  Java_BraveRewardsNativeWorker_OnGetCurrentBalanceReport(env,
        weak_java_brave_rewards_native_worker_.get(env), java_array);
}

void BraveRewardsNativeWorker::Donate(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& publisher_key,
        int amount, bool recurring) {
  if (brave_rewards_service_) {
    brave_rewards_service_->OnTip(
      base::android::ConvertJavaStringToUTF8(env, publisher_key), amount,
        recurring);
    if (!recurring) {
      Java_BraveRewardsNativeWorker_OnOneTimeTip(env,
        weak_java_brave_rewards_native_worker_.get(env));
    }
  }
}

void BraveRewardsNativeWorker::GetAllNotifications(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj) {
  if (!brave_rewards_service_) {
    return;
  }
  brave_rewards::RewardsNotificationService* notification_service =
    brave_rewards_service_->GetNotificationService();
  if (notification_service) {
    notification_service->GetNotifications();
  }
}

void BraveRewardsNativeWorker::DeleteNotification(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& notification_id) {
  if (!brave_rewards_service_) {
    return;
  }
  brave_rewards::RewardsNotificationService* notification_service =
    brave_rewards_service_->GetNotificationService();
  if (notification_service) {
    notification_service->DeleteNotification(
      base::android::ConvertJavaStringToUTF8(env, notification_id));
  }
}

void BraveRewardsNativeWorker::GetGrant(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& promotionId) {
  if (brave_rewards_service_) {
    std::string promotion_id =
      base::android::ConvertJavaStringToUTF8(env, promotionId);
    brave_rewards_service_->ClaimPromotion(promotion_id,
      base::BindOnce(
        &BraveRewardsNativeWorker::OnClaimPromotion,
        base::Unretained(this)));
  }
}

void BraveRewardsNativeWorker::OnClaimPromotion(
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnClaimPromotion(env,
      weak_java_brave_rewards_native_worker_.get(env),
      static_cast<int>(result));
}

base::android::ScopedJavaLocalRef<jobjectArray>
    BraveRewardsNativeWorker::GetCurrentGrant(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      int position) {
  if ((size_t)position > promotions_.size() - 1) {
    return base::android::ScopedJavaLocalRef<jobjectArray>();
  }
  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) <<
      (promotions_[position])->approximate_value;
  std::vector<std::string> values;
  values.push_back(stream.str());
  values.push_back(
    std::to_string((promotions_[position])->expires_at));
  values.push_back(
      std::to_string(static_cast<int>((promotions_[position])->type)));

  return base::android::ToJavaArrayOfStrings(env, values);
}

void BraveRewardsNativeWorker::GetPendingContributionsTotal(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetPendingContributionsTotal(base::Bind(
          &BraveRewardsNativeWorker::OnGetPendingContributionsTotal,
          weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::GetRecurringDonations(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetRecurringTips(base::Bind(
          &BraveRewardsNativeWorker::OnGetRecurringTips,
          weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetRecurringTips(
    ledger::type::PublisherInfoList list) {
  map_recurrent_publishers_.clear();
  for (const auto& item : list) {
    map_recurrent_publishers_[item->id] = item->Clone();
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnRecurringDonationUpdated(env,
        weak_java_brave_rewards_native_worker_.get(env));
}

bool BraveRewardsNativeWorker::IsCurrentPublisherInRecurrentDonations(
    JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& publisher) {
  return map_recurrent_publishers_.find(
    base::android::ConvertJavaStringToUTF8(env, publisher)) !=
      map_recurrent_publishers_.end();
}


void BraveRewardsNativeWorker::GetAutoContributeProperties(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetAutoContributeProperties(
        base::Bind(&BraveRewardsNativeWorker::OnGetAutoContributeProperties,
                   weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnGetAutoContributeProperties(
    ledger::type::AutoContributePropertiesPtr properties) {
  if (properties) {
    auto_contrib_properties_ = std::move(properties);
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnGetAutoContributeProperties(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

bool BraveRewardsNativeWorker::IsAutoContributeEnabled(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (!auto_contrib_properties_) {
    return false;
  }

  return auto_contrib_properties_->enabled_contribute;
}

void BraveRewardsNativeWorker::GetReconcileStamp(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetReconcileStamp(base::Bind(
            &BraveRewardsNativeWorker::OnGetGetReconcileStamp,
            weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::ResetTheWholeState(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->CompleteReset(base::Bind(
           &BraveRewardsNativeWorker::OnResetTheWholeState,
           weak_factory_.GetWeakPtr()));
  } else {
    JNIEnv* env = base::android::AttachCurrentThread();

    Java_BraveRewardsNativeWorker_OnResetTheWholeState(env,
            weak_java_brave_rewards_native_worker_.get(env), false);
  }
}

void BraveRewardsNativeWorker::OnResetTheWholeState(const bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnResetTheWholeState(env,
          weak_java_brave_rewards_native_worker_.get(env), success);
}

double BraveRewardsNativeWorker::GetPublisherRecurrentDonationAmount(
    JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& publisher) {
  double amount(0.0);
  auto it = map_recurrent_publishers_.find(
    base::android::ConvertJavaStringToUTF8(env, publisher));
  if (it != map_recurrent_publishers_.end()) {
    // for Recurrent Donations, the amount is stored in ContentSite::percentage
    amount = it->second->percent;
  }
  return  amount;
}

void BraveRewardsNativeWorker::RemoveRecurring(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& publisher) {
  if (brave_rewards_service_) {
      brave_rewards_service_->RemoveRecurringTip(
        base::android::ConvertJavaStringToUTF8(env, publisher));
  }
}

void BraveRewardsNativeWorker::OnGetGetReconcileStamp(uint64_t timestamp) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnGetReconcileStamp(env,
          weak_java_brave_rewards_native_worker_.get(env), timestamp);
}

void BraveRewardsNativeWorker::OnGetPendingContributionsTotal(double amount) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnGetPendingContributionsTotal(env,
        weak_java_brave_rewards_native_worker_.get(env), amount);
}

void BraveRewardsNativeWorker::OnNotificationAdded(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnNotificationAdded(env,
        weak_java_brave_rewards_native_worker_.get(env),
        base::android::ConvertUTF8ToJavaString(env, notification.id_),
        notification.type_,
        notification.timestamp_,
        base::android::ToJavaArrayOfStrings(env, notification.args_));
}

void BraveRewardsNativeWorker::OnGetAllNotifications(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotificationsList&
      notifications_list) {
  JNIEnv* env = base::android::AttachCurrentThread();

  // Notify about notifications count
  Java_BraveRewardsNativeWorker_OnNotificationsCount(env,
        weak_java_brave_rewards_native_worker_.get(env),
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
    Java_BraveRewardsNativeWorker_OnGetLatestNotification(env,
        weak_java_brave_rewards_native_worker_.get(env),
        base::android::ConvertUTF8ToJavaString(env, iter->id_),
        iter->type_,
        iter->timestamp_,
        base::android::ToJavaArrayOfStrings(env, iter->args_));
  }
}

void BraveRewardsNativeWorker::OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnNotificationDeleted(env,
        weak_java_brave_rewards_native_worker_.get(env),
        base::android::ConvertUTF8ToJavaString(env, notification.id_));
}

void BraveRewardsNativeWorker::OnPromotionFinished(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnGrantFinish(env,
        weak_java_brave_rewards_native_worker_.get(env),
        static_cast<int>(result));
}

int BraveRewardsNativeWorker::GetAdsPerHour(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
  if (!ads_service_) {
    return DEFAULT_ADS_PER_HOUR;
  }
  return ads_service_->GetAdsPerHour();
}

void BraveRewardsNativeWorker::SetAdsPerHour(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jint value) {
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
  if (!ads_service_) {
    return;
  }
  ads_service_->SetAdsPerHour(value);
}

void BraveRewardsNativeWorker::SetAutoContributionAmount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jdouble value) {
  if (brave_rewards_service_) {
    brave_rewards_service_->SetAutoContributionAmount(value);
  }
}

bool BraveRewardsNativeWorker::IsAnonWallet(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  if (brave_rewards_service_) {
    return brave_rewards_service_->OnlyAnonWallet();
  }
  return false;
}

void BraveRewardsNativeWorker::GetExternalWallet(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    auto callback = base::Bind(
        &BraveRewardsNativeWorker::OnGetExternalWallet,
        base::Unretained(this));
    brave_rewards_service_->GetExternalWallet(
        brave_rewards_service_->GetExternalWalletType(), callback);
  }
}

void BraveRewardsNativeWorker::OnGetExternalWallet(
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  std::string json_wallet;
  if (!wallet) {
    json_wallet = "";
  } else {
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey("token", wallet->token);
    dict.SetStringKey("address", wallet->address);

    // enum class WalletStatus : int32_t
    dict.SetIntKey("status", static_cast<int32_t>(wallet->status));
    dict.SetStringKey("verify_url", wallet->verify_url);
    dict.SetStringKey("add_url", wallet->add_url);
    dict.SetStringKey("withdraw_url", wallet->withdraw_url);
    dict.SetStringKey("user_name", wallet->user_name);
    dict.SetStringKey("account_url", wallet->account_url);
    dict.SetStringKey("login_url", wallet->login_url);
    base::JSONWriter::Write(dict, &json_wallet);
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnGetExternalWallet(env,
      weak_java_brave_rewards_native_worker_.get(env),
      static_cast<int>(result),
      base::android::ConvertUTF8ToJavaString(env, json_wallet));
}

void BraveRewardsNativeWorker::DisconnectWallet(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& wallet_type) {
  if (brave_rewards_service_) {
    std::string str_wallet_type =
        base::android::ConvertJavaStringToUTF8(env, wallet_type);
    brave_rewards_service_->DisconnectWallet(str_wallet_type);
  }
}

void BraveRewardsNativeWorker::OnDisconnectWallet(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    const std::string& wallet_type) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnDisconnectWallet(env,
        weak_java_brave_rewards_native_worker_.get(env),
        static_cast<int>(result),
        base::android::ConvertUTF8ToJavaString(env, wallet_type));
}

void BraveRewardsNativeWorker::ProcessRewardsPageUrl(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& path,
        const base::android::JavaParamRef<jstring>& query) {
  if (brave_rewards_service_) {
    std::string cpath = base::android::ConvertJavaStringToUTF8(env, path);
    std::string cquery = base::android::ConvertJavaStringToUTF8(env, query);
    auto callback = base::Bind(
        &BraveRewardsNativeWorker::OnProcessRewardsPageUrl,
        base::Unretained(this));
    brave_rewards_service_->ProcessRewardsPageUrl(cpath, cquery, callback);
  }
}

void BraveRewardsNativeWorker::OnProcessRewardsPageUrl(
    const ledger::type::Result result,
    const std::string& wallet_type,
    const std::string& action,
    const base::flat_map<std::string, std::string>& args) {
  std::string json_args = StdStrStrMapToJsonString(args);
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnProcessRewardsPageUrl(env,
        weak_java_brave_rewards_native_worker_.get(env),
        static_cast<int>(result),
        base::android::ConvertUTF8ToJavaString(env, wallet_type),
        base::android::ConvertUTF8ToJavaString(env, action),
        base::android::ConvertUTF8ToJavaString(env, json_args));
}

std::string BraveRewardsNativeWorker::StdStrStrMapToJsonString(
    const base::flat_map<std::string, std::string>& args) {
    std::string json_args;
    base::Value dict(base::Value::Type::DICTIONARY);
    for (const auto & item : args) {
      dict.SetStringKey(item.first, item.second);
    }
    base::JSONWriter::Write(dict, &json_args);
    return json_args;
}

void BraveRewardsNativeWorker::RecoverWallet(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& pass_phrase) {
  if (brave_rewards_service_) {
    brave_rewards_service_->RecoverWallet(
        base::android::ConvertJavaStringToUTF8(env, pass_phrase));
  }
}

void BraveRewardsNativeWorker::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnRecoverWallet(
      env, weak_java_brave_rewards_native_worker_.get(env),
      static_cast<int>(result));
}

void BraveRewardsNativeWorker::RefreshPublisher(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& publisher_key) {
  if (!brave_rewards_service_) {
    NOTREACHED();
    return;
  }
  brave_rewards_service_->RefreshPublisher(
      base::android::ConvertJavaStringToUTF8(env, publisher_key),
      base::BindOnce(&BraveRewardsNativeWorker::OnRefreshPublisher,
                     base::Unretained(this)));
}

void BraveRewardsNativeWorker::OnRefreshPublisher(
    const ledger::type::PublisherStatus status,
    const std::string& publisher_key) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnRefreshPublisher(
      env, weak_java_brave_rewards_native_worker_.get(env),
      static_cast<int>(status),
      base::android::ConvertUTF8ToJavaString(env, publisher_key));
}

void BraveRewardsNativeWorker::SetAutoContributeEnabled(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    bool isAutoContributeEnabled) {
  if (brave_rewards_service_) {
    brave_rewards_service_->SetAutoContributeEnabled(isAutoContributeEnabled);
  }
}

static void JNI_BraveRewardsNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveRewardsNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
