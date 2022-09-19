/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_ANDROID_BRAVE_REWARDS_NATIVE_WORKER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_ANDROID_BRAVE_REWARDS_NATIVE_WORKER_H_

#include <jni.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"

namespace brave_rewards {
class RewardsService;
}

namespace chrome {
namespace android {

typedef std::map<uint64_t, ledger::mojom::PublisherInfoPtr> PublishersInfoMap;

class BraveRewardsNativeWorker : public brave_rewards::RewardsServiceObserver,
    public brave_rewards::RewardsServicePrivateObserver,
    public brave_rewards::RewardsNotificationServiceObserver {
 public:
    BraveRewardsNativeWorker(JNIEnv* env,
        const base::android::JavaRef<jobject>& obj);
    ~BraveRewardsNativeWorker() override;

    void Destroy(JNIEnv* env);

    void GetRewardsParameters(JNIEnv* env);

    void GetPublisherInfo(JNIEnv* env,
                          int tabId,
                          const base::android::JavaParamRef<jstring>& host);

    base::android::ScopedJavaLocalRef<jstring> GetWalletBalance(JNIEnv* env);

    base::android::ScopedJavaLocalRef<jstring> GetExternalWalletType(
        JNIEnv* env);

    void GetAdsAccountStatement(JNIEnv* env);

    base::android::ScopedJavaLocalRef<jdoubleArray> GetTipChoices(JNIEnv* env);

    double GetWalletRate(JNIEnv* env);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherURL(JNIEnv* env,
                                                               uint64_t tabId);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherFavIconURL(
        JNIEnv* env,
        uint64_t tabId);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherName(JNIEnv* env,
                                                                uint64_t tabId);
    base::android::ScopedJavaLocalRef<jstring> GetCaptchaSolutionURL(
        JNIEnv* env,
        const base::android::JavaParamRef<jstring>& paymentId,
        const base::android::JavaParamRef<jstring>& captchaId);

    base::android::ScopedJavaLocalRef<jstring> GetPublisherId(JNIEnv* env,
        uint64_t tabId);

    int GetPublisherPercent(JNIEnv* env, uint64_t tabId);

    bool GetPublisherExcluded(JNIEnv* env, uint64_t tabId);

    int GetPublisherStatus(JNIEnv* env, uint64_t tabId);

    void GetCurrentBalanceReport(JNIEnv* env);

    void IncludeInAutoContribution(JNIEnv* env, uint64_t tabId, bool exclude);

    void RemovePublisherFromMap(JNIEnv* env, uint64_t tabId);

    void Donate(JNIEnv* env,
                const base::android::JavaParamRef<jstring>& publisher_key,
                double amount,
                bool recurring);

    void GetAllNotifications(JNIEnv* env);

    void DeleteNotification(JNIEnv* env,
        const base::android::JavaParamRef<jstring>& notification_id);

    void GetGrant(JNIEnv* env,
                  const base::android::JavaParamRef<jstring>& promotionId);

    base::android::ScopedJavaLocalRef<jobjectArray> GetCurrentGrant(
        JNIEnv* env,
        int position);

    void GetPendingContributionsTotal(JNIEnv* env);

    void GetRecurringDonations(JNIEnv* env);

    bool IsCurrentPublisherInRecurrentDonations(JNIEnv* env,
        const base::android::JavaParamRef<jstring>& publisher);

    void GetAutoContributeProperties(JNIEnv* env);

    bool IsAutoContributeEnabled(JNIEnv* env);

    void GetReconcileStamp(JNIEnv* env);

    void ResetTheWholeState(JNIEnv* env);

    double GetPublisherRecurrentDonationAmount(JNIEnv* env,
        const base::android::JavaParamRef<jstring>& publisher);

    void RemoveRecurring(JNIEnv* env,
        const base::android::JavaParamRef<jstring>& publisher);

    void FetchGrants(JNIEnv* env);

    int GetAdsPerHour(JNIEnv* env);

    void SetAdsPerHour(JNIEnv* env, jint value);

    void SetAutoContributionAmount(JNIEnv* env, jdouble value);

    void GetAutoContributionAmount(JNIEnv* env);

    void GetExternalWallet(JNIEnv* env);

    void GetPublisherBanner(
        JNIEnv* env,
        const base::android::JavaParamRef<jstring>& publisher_key);

    void DisconnectWallet(JNIEnv* env);

    void RecoverWallet(JNIEnv* env,
                       const base::android::JavaParamRef<jstring>& pass_phrase);

    void RefreshPublisher(
        JNIEnv* env,
        const base::android::JavaParamRef<jstring>& publisher_key);

    void OnResetTheWholeState(const bool success);

    void OnGetGetReconcileStamp(uint64_t timestamp);

    void OnGetAutoContributeProperties(
        ledger::mojom::AutoContributePropertiesPtr properties);

    void OnGetAutoContributionAmount(double auto_contribution_amount);

    void OnGetPendingContributionsTotal(double amount);

    void OnPanelPublisherInfo(brave_rewards::RewardsService* rewards_service,
                              const ledger::mojom::Result result,
                              const ledger::mojom::PublisherInfo* info,
                              uint64_t tabId) override;

    void OnGetCurrentBalanceReport(
        brave_rewards::RewardsService* rewards_service,
        const ledger::mojom::Result result,
        ledger::mojom::BalanceReportInfoPtr report);

    void OnGetRewardsParameters(brave_rewards::RewardsService* rewards_service,
                                ledger::mojom::RewardsParametersPtr parameters);

    void OnUnblindedTokensReady(
        brave_rewards::RewardsService* rewards_service) override;

    void OnReconcileComplete(
        brave_rewards::RewardsService* rewards_service,
        const ledger::mojom::Result result,
        const std::string& contribution_id,
        const double amount,
        const ledger::mojom::RewardsType type,
        const ledger::mojom::ContributionProcessor processor) override;

    void OnPendingContributionSaved(
        brave_rewards::RewardsService* rewards_service,
        const ledger::mojom::Result result) override;

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

    void OnPromotionFinished(brave_rewards::RewardsService* rewards_service,
                             const ledger::mojom::Result result,
                             ledger::mojom::PromotionPtr promotion) override;

    void OnGetRecurringTips(std::vector<ledger::mojom::PublisherInfoPtr> list);

    bool IsRewardsEnabled(JNIEnv* env);

    void OnClaimPromotion(const ledger::mojom::Result result,
                          ledger::mojom::PromotionPtr promotion);

    void OnGetExternalWallet(const ledger::mojom::Result result,
                             ledger::mojom::ExternalWalletPtr wallet);

    void onPublisherBanner(ledger::mojom::PublisherBannerPtr wallet);

    void OnOneTimeTip(ledger::mojom::Result result);

    void OnDisconnectWallet(brave_rewards::RewardsService* rewards_service,
                            const ledger::mojom::Result result,
                            const std::string& wallet_type) override;

    void OnRecoverWallet(brave_rewards::RewardsService* rewards_service,
                         const ledger::mojom::Result result) override;

    void OnRefreshPublisher(const ledger::mojom::PublisherStatus status,
                            const std::string& publisher_key);
    void SetAutoContributeEnabled(
        JNIEnv* env,
        bool isAutoContributeEnabled);
    void StartProcess(JNIEnv* env);

 private:
    std::string StdStrStrMapToJsonString(
        const base::flat_map<std::string, std::string>& args);

    void OnBalance(const ledger::mojom::Result result,
                   ledger::mojom::BalancePtr balance);

    void OnStartProcess();

    void OnGetAdsAccountStatement(ads::mojom::StatementInfoPtr statement);

    JavaObjectWeakGlobalRef weak_java_brave_rewards_native_worker_;
    raw_ptr<brave_rewards::RewardsService> brave_rewards_service_ = nullptr;
    ledger::mojom::RewardsParameters parameters_;
    ledger::mojom::Balance balance_;
    ledger::mojom::AutoContributePropertiesPtr auto_contrib_properties_;
    PublishersInfoMap map_publishers_info_;
    std::map<std::string, ledger::mojom::PublisherInfoPtr>
        map_recurrent_publishers_;
    std::map<std::string, std::string> addresses_;
    std::vector<ledger::mojom::PromotionPtr> promotions_;
    base::WeakPtrFactory<BraveRewardsNativeWorker> weak_factory_;
};
}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_ANDROID_BRAVE_REWARDS_NATIVE_WORKER_H_
