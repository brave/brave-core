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
#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

namespace brave_rewards {
class RewardsService;
}

namespace chrome {
namespace android {

typedef std::map<uint64_t, brave_rewards::mojom::PublisherInfoPtr>
    PublishersInfoMap;

class BraveRewardsNativeWorker
    : public brave_rewards::RewardsServiceObserver,
      public brave_rewards::RewardsNotificationServiceObserver {
 public:
  BraveRewardsNativeWorker(JNIEnv* env,
                           const base::android::JavaRef<jobject>& obj);
  ~BraveRewardsNativeWorker() override;

  void Destroy(JNIEnv* env);

  bool IsSupported(JNIEnv* env);
  bool IsSupportedSkipRegionCheck(JNIEnv* env);

  std::string StringifyResult(
      brave_rewards::mojom::CreateRewardsWalletResult result);

  bool IsRewardsEnabled(JNIEnv* env);

  bool ShouldShowSelfCustodyInvite(JNIEnv* env);

  void CreateRewardsWallet(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& country_code);

  void GetRewardsParameters(JNIEnv* env);

  double GetVbatDeadline(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jstring> GetPayoutStatus(JNIEnv* env);

  void GetUserType(JNIEnv* env);

  void FetchBalance(JNIEnv* env);

  void GetPublisherInfo(JNIEnv* env,
                        int tabId,
                        const base::android::JavaParamRef<jstring>& host);

  base::android::ScopedJavaLocalRef<jstring> GetWalletBalance(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jstring> GetExternalWalletType(JNIEnv* env);

  void GetAdsAccountStatement(JNIEnv* env);

  bool CanConnectAccount(JNIEnv* env);

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
  base::android::ScopedJavaLocalRef<jstring> GetAttestationURL(JNIEnv* env);
  base::android::ScopedJavaLocalRef<jstring> GetAttestationURLWithPaymentId(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& paymentId);

  base::android::ScopedJavaLocalRef<jstring> GetPublisherId(JNIEnv* env,
                                                            uint64_t tabId);

  int GetPublisherPercent(JNIEnv* env, uint64_t tabId);

  bool GetPublisherExcluded(JNIEnv* env, uint64_t tabId);

  int GetPublisherStatus(JNIEnv* env, uint64_t tabId);

  void GetCurrentBalanceReport(JNIEnv* env);

  void RemovePublisherFromMap(JNIEnv* env, uint64_t tabId);

  void Donate(JNIEnv* env,
              const base::android::JavaParamRef<jstring>& publisher_key,
              double amount,
              bool recurring);

  void GetAllNotifications(JNIEnv* env);

  void DeleteNotification(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& notification_id);

  void GetRecurringDonations(JNIEnv* env);

  bool IsCurrentPublisherInRecurrentDonations(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& publisher);

  void GetReconcileStamp(JNIEnv* env);

  void ResetTheWholeState(JNIEnv* env);

  double GetPublisherRecurrentDonationAmount(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& publisher);

  void RemoveRecurring(JNIEnv* env,
                       const base::android::JavaParamRef<jstring>& publisher);

  int GetAdsPerHour(JNIEnv* env);

  void SetAdsPerHour(JNIEnv* env, jint value);

  void GetExternalWallet(JNIEnv* env);

  bool IsTermsOfServiceUpdateRequired(JNIEnv* env);

  void AcceptTermsOfServiceUpdate(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jstring> GetCountryCode(JNIEnv* env);

  void GetAvailableCountries(JNIEnv* env);

  void GetPublisherBanner(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& publisher_key);

  void GetPublishersVisitedCount(JNIEnv* env);

  void OnGetPublishersVisitedCount(int count);

  void DisconnectWallet(JNIEnv* env);

  void RefreshPublisher(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& publisher_key);

  void RecordPanelTrigger(JNIEnv* env);

  void OnCreateRewardsWallet(
      brave_rewards::mojom::CreateRewardsWalletResult result);

  void OnCompleteReset(const bool success) override;

  void OnResetTheWholeState(const bool success);

  void OnGetGetReconcileStamp(uint64_t timestamp);

  void OnPanelPublisherInfo(brave_rewards::RewardsService* rewards_service,
                            const brave_rewards::mojom::Result result,
                            const brave_rewards::mojom::PublisherInfo* info,
                            uint64_t tabId) override;

  void OnGetCurrentBalanceReport(
      brave_rewards::RewardsService* rewards_service,
      const brave_rewards::mojom::Result result,
      brave_rewards::mojom::BalanceReportInfoPtr report);

  void OnGetRewardsParameters(
      brave_rewards::RewardsService* rewards_service,
      brave_rewards::mojom::RewardsParametersPtr parameters);

  void OnTermsOfServiceUpdateAccepted() override;

  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      const brave_rewards::mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const brave_rewards::mojom::RewardsType type,
      const brave_rewards::mojom::ContributionProcessor processor) override;

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

  void OnGetRecurringTips(
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list);

  void OnGetExternalWallet(brave_rewards::mojom::ExternalWalletPtr wallet);

  void OnGetAvailableCountries(std::vector<std::string> countries);

  void onPublisherBanner(brave_rewards::mojom::PublisherBannerPtr wallet);

  void OnSendContribution(bool result);

  void OnExternalWalletConnected() override;

  void OnExternalWalletLoggedOut() override;

  void OnExternalWalletReconnected() override;

  void OnRefreshPublisher(const brave_rewards::mojom::PublisherStatus status,
                          const std::string& publisher_key);

 private:
  std::string StdStrStrMapToJsonString(
      const base::flat_map<std::string, std::string>& args);

  void OnGetUserType(const brave_rewards::mojom::UserType user_type);

  void OnBalance(brave_rewards::mojom::BalancePtr balance);

  void OnGetAdsAccountStatement(brave_ads::mojom::StatementInfoPtr statement);

  JavaObjectWeakGlobalRef weak_java_brave_rewards_native_worker_;
  raw_ptr<brave_rewards::RewardsService> brave_rewards_service_ = nullptr;
  brave_rewards::mojom::RewardsParametersPtr parameters_;
  brave_rewards::mojom::Balance balance_;
  PublishersInfoMap map_publishers_info_;
  std::map<std::string, brave_rewards::mojom::PublisherInfoPtr>
      map_recurrent_publishers_;
  std::map<std::string, std::string> addresses_;
  base::ScopedObservation<brave_rewards::RewardsService,
                          brave_rewards::RewardsServiceObserver>
      rewards_service_observation_{this};
  base::ScopedObservation<brave_rewards::RewardsNotificationService,
                          brave_rewards::RewardsNotificationServiceObserver>
      rewards_notification_service_observation_{this};
  base::WeakPtrFactory<BraveRewardsNativeWorker> weak_factory_;
};

}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_ANDROID_BRAVE_REWARDS_NATIVE_WORKER_H_
