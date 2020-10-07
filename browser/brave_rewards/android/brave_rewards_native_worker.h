/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_ANDROID_BRAVE_REWARDS_NATIVE_WORKER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_ANDROID_BRAVE_REWARDS_NATIVE_WORKER_H_

#include <jni.h>
#include <memory>
#include <map>
#include <string>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"

namespace brave_rewards {
class RewardsService;
}

namespace chrome {
namespace android {

typedef std::map<uint64_t, ledger::type::PublisherInfoPtr> PublishersInfoMap;

class BraveRewardsNativeWorker : public brave_rewards::RewardsServiceObserver,
    public brave_rewards::RewardsServicePrivateObserver,
    public brave_rewards::RewardsNotificationServiceObserver {
 public:
    BraveRewardsNativeWorker(JNIEnv* env,
        const base::android::JavaRef<jobject>& obj);
    ~BraveRewardsNativeWorker() override;

    void Destroy(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void CreateWallet(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void OnCreateWallet(const ledger::type::Result result);

    void WalletExist(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void GetRewardsParameters(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void GetPublisherInfo(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller, int tabId,
        const base::android::JavaParamRef<jstring>& host);

    base::android::ScopedJavaLocalRef<jstring> GetWalletBalance(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    double GetWalletRate(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherURL(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherFavIconURL(
        JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
        uint64_t tabId);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherName(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherId(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId);

    int GetPublisherPercent(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        uint64_t tabId);

    bool GetPublisherExcluded(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId);

    int GetPublisherStatus(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId);

    void GetCurrentBalanceReport(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    void IncludeInAutoContribution(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId,
        bool exclude);

    void RemovePublisherFromMap(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, uint64_t tabId);

    void Donate(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& publisher_key, int amount,
        bool recurring);

    void GetAllNotifications(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    void DeleteNotification(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& notification_id);

    void GetGrant(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& promotionId);

    base::android::ScopedJavaLocalRef<jobjectArray> GetCurrentGrant(
        JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
        int position);

    void GetPendingContributionsTotal(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    void GetRecurringDonations(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    bool IsCurrentPublisherInRecurrentDonations(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& publisher);

    void SetRewardsMainEnabled(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj, bool enabled);
    void GetRewardsMainEnabled(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    void GetAutoContributeProperties(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    bool IsAutoContributeEnabled(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    void GetReconcileStamp(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    void ResetTheWholeState(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    double GetPublisherRecurrentDonationAmount(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& publisher);

    void RemoveRecurring(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& publisher);

    void FetchGrants(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    int GetAdsPerHour(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj);

    void SetAdsPerHour(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj,
                       jint value);

    void GetExternalWallet(
        JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj);

    void DisconnectWallet(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& wallet_type);

    void ProcessRewardsPageUrl(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& path,
        const base::android::JavaParamRef<jstring>& query);

    void RecoverWallet(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj,
                       const base::android::JavaParamRef<jstring>& pass_phrase);

    void RefreshPublisher(
        JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj,
        const base::android::JavaParamRef<jstring>& publisher_key);

    void OnResetTheWholeState(const bool success);

    void OnGetGetReconcileStamp(uint64_t timestamp);

    void OnGetAutoContributeProperties(
        ledger::type::AutoContributePropertiesPtr properties);

    void OnGetRewardsMainEnabled(bool enabled);

    void OnGetPendingContributionsTotal(double amount);

    void OnIsWalletCreated(bool created);

    void OnWalletInitialized(
        brave_rewards::RewardsService* rewards_service,
        const ledger::type::Result result) override;

    void OnPanelPublisherInfo(
        brave_rewards::RewardsService* rewards_service,
        const ledger::type::Result result,
        const ledger::type::PublisherInfo* info,
        uint64_t tabId) override;

    void OnGetCurrentBalanceReport(
        brave_rewards::RewardsService* rewards_service,
        const ledger::type::Result result,
        ledger::type::BalanceReportInfoPtr report);

    void OnGetRewardsParameters(
        brave_rewards::RewardsService* rewards_service,
        ledger::type::RewardsParametersPtr parameters);

    void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) override;

    void OnGetAllNotifications(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotificationsList&
          notifications_list) override;

    void OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) override;

    void OnPromotionFinished(
        brave_rewards::RewardsService* rewards_service,
        const ledger::type::Result result,
        ledger::type::PromotionPtr promotion) override;

    void OnGetRecurringTips(ledger::type::PublisherInfoList list);

    void OnRewardsMainEnabled(brave_rewards::RewardsService* rewards_service,
        bool rewards_main_enabled) override;

    bool IsAnonWallet(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void OnClaimPromotion(
        const ledger::type::Result result,
        ledger::type::PromotionPtr promotion);

    void OnGetExternalWallet(
        const ledger::type::Result result,
        ledger::type::UpholdWalletPtr wallet);

    void OnDisconnectWallet(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& wallet_type) override;

    void OnProcessRewardsPageUrl(
        const ledger::type::Result result,
        const std::string& wallet_type,
        const std::string& action,
        const std::map<std::string, std::string>& args);

    void OnRecoverWallet(
        brave_rewards::RewardsService* rewards_service,
        const ledger::type::Result result) override;

    void OnRefreshPublisher(
        const ledger::type::PublisherStatus status,
        const std::string& publisher_key);

 private:
    std::string StdStrStrMapToJsonString(
        const std::map<std::string, std::string>& args);

    void OnBalance(
        const ledger::type::Result result,
        ledger::type::BalancePtr balance);
    JavaObjectWeakGlobalRef weak_java_brave_rewards_native_worker_;
    brave_rewards::RewardsService* brave_rewards_service_;
    ledger::type::RewardsParameters parameters_;
    ledger::type::Balance balance_;
    ledger::type::AutoContributePropertiesPtr auto_contrib_properties_;
    PublishersInfoMap map_publishers_info_;
    std::map<std::string, ledger::type::PublisherInfoPtr>
      map_recurrent_publishers_;
    std::map<std::string, std::string> addresses_;
    ledger::type::PromotionList promotions_;
    base::WeakPtrFactory<BraveRewardsNativeWorker> weak_factory_;
};
}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_ANDROID_BRAVE_REWARDS_NATIVE_WORKER_H_
