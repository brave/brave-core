/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/android/brave_rewards_native_worker.h"

#include <iomanip>
#include <string>
#include <vector>
#include <utility>

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

void BraveRewardsNativeWorker::CreateWallet(JNIEnv* env, const
        base::android::JavaParamRef<jobject>& jcaller) {
  if (brave_rewards_service_) {
    brave_rewards_service_->CreateWallet(base::Bind(
            &BraveRewardsNativeWorker::OnCreateWallet,
            weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::OnCreateWallet(int32_t result) {
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
      int error_code,
      const ledger::PublisherInfo* info,
      uint64_t tabId) {
  if (!info) {
    return;
  }
  ledger::PublisherInfoPtr pi = info->Clone();
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
    res = iter->second->excluded == ledger::PublisherExclude::EXCLUDED;
  }

  return res;
}

int BraveRewardsNativeWorker::GetPublisherStatus(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId) {
  int res = static_cast<int>(ledger::PublisherStatus::NOT_VERIFIED);
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
      iter->second->excluded = ledger::PublisherExclude::EXCLUDED;
    } else {
      iter->second->excluded = ledger::PublisherExclude::INCLUDED;
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

void BraveRewardsNativeWorker::OnWalletInitialized(
  brave_rewards::RewardsService* rewards_service, int32_t error_code) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnWalletInitialized(env,
        weak_java_brave_rewards_native_worker_.get(env), error_code);
}

void BraveRewardsNativeWorker::OnGetRewardsParameters(
    brave_rewards::RewardsService* rewards_service,
    std::unique_ptr<brave_rewards::RewardsParameters> parameters) {
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
    int32_t result,
    std::unique_ptr<brave_rewards::Balance> balance) {
  if (result == 0 && balance) {
    balance_ = *balance;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnRewardsParameters(
      env, weak_java_brave_rewards_native_worker_.get(env), 0);
}

base::android::ScopedJavaLocalRef<jstring>
    BraveRewardsNativeWorker::GetWalletBalance(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  std::string json_balance = balance_.toJson();
  return base::android::ConvertUTF8ToJavaString(env, json_balance);
}

double BraveRewardsNativeWorker::GetWalletRate(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  return parameters_.rate;
}

void BraveRewardsNativeWorker::WalletExist(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller) {
  if (brave_rewards_service_) {
    brave_rewards_service_->IsWalletCreated(
      base::Bind(&BraveRewardsNativeWorker::OnIsWalletCreated,
          weak_factory_.GetWeakPtr()));
  }
}

void BraveRewardsNativeWorker::FetchGrants(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->FetchPromotions();
  }
}

void BraveRewardsNativeWorker::OnIsWalletCreated(bool created) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnIsWalletCreated(env,
        weak_java_brave_rewards_native_worker_.get(env), created);
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
        const int32_t result,
        const brave_rewards::BalanceReport& balance_report) {
  std::vector<double> values;
  values.push_back(balance_report.grants);
  values.push_back(balance_report.earning_from_ads);
  values.push_back(balance_report.auto_contribute);
  values.push_back(balance_report.recurring_donation);
  values.push_back(balance_report.one_time_donation);

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jdoubleArray> java_array =
      base::android::ToJavaDoubleArray(env, values);

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

void BraveRewardsNativeWorker::OnClaimPromotion(const int32_t result,
        std::unique_ptr<brave_rewards::Promotion> promotion) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnClaimPromotion(env,
      weak_java_brave_rewards_native_worker_.get(env), result);
}

base::android::ScopedJavaLocalRef<jobjectArray>
    BraveRewardsNativeWorker::GetCurrentGrant(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      int position) {
  if ((size_t)position > promotions_.size() - 1) {
    return base::android::ScopedJavaLocalRef<jobjectArray>();
  }
  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << promotions_[position].amount;
  std::vector<std::string> values;
  values.push_back(stream.str());
  values.push_back(
    std::to_string(promotions_[position].expires_at));
  values.push_back(std::to_string(promotions_[position].type));

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
        std::unique_ptr<brave_rewards::ContentSiteList> list) {
  map_recurrent_publishers_.clear();
  if (list) {
    for (size_t i = 0; i < list->size(); i++) {
      map_recurrent_publishers_[(*list)[i].id] = (*list)[i];
    }
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
    std::unique_ptr<brave_rewards::AutoContributeProps> props) {
  if (props) {
    auto_contrib_properties_ = *props;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnGetAutoContributeProperties(
      env, weak_java_brave_rewards_native_worker_.get(env));
}

bool BraveRewardsNativeWorker::IsAutoContributeEnabled(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  return auto_contrib_properties_.enabled_contribute;
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
    amount = it->second.percentage;
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
    const uint32_t result,
    brave_rewards::Promotion promotion) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnGrantFinish(env,
        weak_java_brave_rewards_native_worker_.get(env), result);
}

void BraveRewardsNativeWorker::SetRewardsMainEnabled(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj, bool enabled) {
  if (brave_rewards_service_) {
    brave_rewards_service_->SetRewardsMainEnabled(enabled);
  }
}

void BraveRewardsNativeWorker::OnRewardsMainEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool rewards_main_enabled) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnRewardsMainEnabled(env,
    weak_java_brave_rewards_native_worker_.get(env), rewards_main_enabled);
}

void BraveRewardsNativeWorker::GetRewardsMainEnabled(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (brave_rewards_service_) {
    brave_rewards_service_->GetRewardsMainEnabled(base::Bind(
      &BraveRewardsNativeWorker::OnGetRewardsMainEnabled,
      base::Unretained(this)));
  }
}

void BraveRewardsNativeWorker::OnGetRewardsMainEnabled(
    bool enabled) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_BraveRewardsNativeWorker_OnGetRewardsMainEnabled(env,
    weak_java_brave_rewards_native_worker_.get(env), enabled);
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

bool BraveRewardsNativeWorker::IsAnonWallet(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  if (brave_rewards_service_) {
    return brave_rewards_service_->OnlyAnonWallet();
  }
  return false;
}

void BraveRewardsNativeWorker::GetExternalWallet(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& wallet_type) {
  if (brave_rewards_service_) {
    std::string str_wallet_type =
        base::android::ConvertJavaStringToUTF8(env, wallet_type);
    auto callback = base::Bind(
        &BraveRewardsNativeWorker::OnGetExternalWallet,
        base::Unretained(this));
    brave_rewards_service_->GetExternalWallet(str_wallet_type, callback);
  }
}

void BraveRewardsNativeWorker::OnGetExternalWallet(int32_t result,
        std::unique_ptr<brave_rewards::ExternalWallet> wallet) {
  std::string json_wallet = wallet->toJson();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnGetExternalWallet(env,
      weak_java_brave_rewards_native_worker_.get(env), result,
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
    int32_t result, const std::string& wallet_type) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnDisconnectWallet(env,
        weak_java_brave_rewards_native_worker_.get(env), result,
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

void BraveRewardsNativeWorker::OnProcessRewardsPageUrl(int32_t result,
        const std::string& wallet_type, const std::string& action,
        const std::map<std::string, std::string>& args) {
  std::string json_args = StdStrStrMapToJsonString(args);
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnProcessRewardsPageUrl(env,
        weak_java_brave_rewards_native_worker_.get(env), result,
        base::android::ConvertUTF8ToJavaString(env, wallet_type),
        base::android::ConvertUTF8ToJavaString(env, action),
        base::android::ConvertUTF8ToJavaString(env, json_args));
}

std::string BraveRewardsNativeWorker::StdStrStrMapToJsonString(
    const std::map<std::string, std::string>& args) {
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
    const int32_t result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveRewardsNativeWorker_OnRecoverWallet(
      env, weak_java_brave_rewards_native_worker_.get(env), result);
}

static void JNI_BraveRewardsNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveRewardsNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
