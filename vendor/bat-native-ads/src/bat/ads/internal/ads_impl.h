/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_IMPL_H_
#define BAT_ADS_INTERNAL_ADS_IMPL_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "bat/ads/ads.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/classification/page_classifier/page_classifier.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_classifier.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/server/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/internal/server/redeem_unblinded_token/redeem_unblinded_token_delegate.h"
#include "bat/ads/internal/server/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/internal/wallet/wallet_info.h"
#include "bat/ads/mojom.h"

namespace ads {

class AdConversions;
class AdNotifications;
class AdRewards;
class Bundle;
class Confirmations;
class ConfirmationType;
class ExclusionRule;
class GetCatalog;
class PermissionRule;
class RedeemUnblindedPaymentTokens;
class RedeemUnblindedToken;
class RefillUnblindedTokens;
class SubdivisionTargeting;
struct AdNotificationInfo;
struct AdsHistory;

namespace database {
class Initialize;
}  // namespace database

class AdsImpl
    : public Ads,
      public RedeemUnblindedPaymentTokensDelegate,
      public RedeemUnblindedTokenDelegate,
      public RefillUnblindedTokensDelegate {
 public:
  explicit AdsImpl(
      AdsClient* ads_client);

  ~AdsImpl() override;

  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  AdConversions* get_ad_conversions() const {
    return ad_conversions_.get();
  }

  AdNotifications* get_ad_notifications() const {
    return ad_notifications_.get();
  }

  AdRewards* get_ad_rewards() const {
    return ad_rewards_.get();
  }

  AdsClient* get_ads_client() const {
    return ads_client_;
  }

  Bundle* get_bundle() const {
    return bundle_.get();
  }

  classification::PageClassifier* get_page_classifier() const {
    return page_classifier_.get();
  }

  classification::PurchaseIntentClassifier*
  get_purchase_intent_classifier() const {
    return purchase_intent_classifier_.get();
  }

  Client* get_client() const {
    return client_.get();
  }

  Confirmations* get_confirmations() const {
    return confirmations_.get();
  }

  GetCatalog* get_get_catalog() const {
    return get_catalog_.get();
  }

  RedeemUnblindedPaymentTokens* get_redeem_unblinded_payment_tokens() const {
    return redeem_unblinded_payment_tokens_.get();
  }

  RedeemUnblindedToken* get_redeem_unblinded_token() const {
    return redeem_unblinded_token_.get();
  }

  RefillUnblindedTokens* get_refill_unblinded_tokens() const {
    return refill_unblinded_tokens_.get();
  }

  SubdivisionTargeting* get_subdivision_targeting() const {
    return subdivision_targeting_.get();
  }

  void Initialize(
      InitializeCallback callback) override;
  void InitializeStep2(
      const Result result,
      InitializeCallback callback);
  void InitializeStep3(
      const Result result,
      InitializeCallback callback);
  void InitializeStep4(
      const Result result,
      InitializeCallback callback);
  void InitializeStep5(
      const Result result,
      InitializeCallback callback);
  void InitializeStep6(
      const Result result,
      InitializeCallback callback);
  bool IsInitialized();

  void Shutdown(
      ShutdownCallback callback) override;

  bool is_foreground_ = false;
  void OnForeground() override;
  void OnBackground() override;
  bool IsForeground() const;

  void OnIdle() override;
  void OnUnIdle() override;

  std::set<int32_t> media_playing_;
  void OnMediaPlaying(
      const int32_t tab_id) override;
  void OnMediaStopped(
      const int32_t tab_id) override;
  bool IsMediaPlaying() const;

  bool GetAdNotification(
      const std::string& uuid,
      AdNotificationInfo* info) override;
  void OnAdNotificationEvent(
      const std::string& uuid,
      const AdNotificationEventType event_type) override;

  bool ShouldNotDisturb() const;

  int32_t get_active_tab_id() const;
  std::string get_active_tab_url() const;
  std::string get_previous_tab_url() const;

  void OnTabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_browser_active,
      const bool is_incognito) override;
  void OnTabClosed(
      const int32_t tab_id) override;

  void OnWalletUpdated(
      const std::string& payment_id,
      const std::string& private_key) override;

  void RemoveAllHistory(
      RemoveAllHistoryCallback callback) override;

  AdsHistory GetAdsHistory(
      const AdsHistory::FilterType filter_type,
      const AdsHistory::SortType sort_type,
      const uint64_t from_timestamp,
      const uint64_t to_timestamp) override;

  AdContent::LikeAction ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContent::LikeAction& action) override;
  AdContent::LikeAction ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContent::LikeAction& action) override;
  CategoryContent::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContent::OptAction& action) override;
  CategoryContent::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContent::OptAction& action) override;
  bool ToggleSaveAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool saved) override;
  bool ToggleFlagAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool flagged) override;

  void ChangeLocale(
      const std::string& locale) override;

  void OnUserModelUpdated(
      const std::string& id) override;

  void OnAdsSubdivisionTargetingCodeHasChanged() override;

  void OnPageLoaded(
      const int32_t tab_id,
      const std::string& original_url,
      const std::string& url,
      const std::string& content) override;

  void MaybeSustainAdNotification(
      const int32_t tab_id,
      const std::string& url);

  classification::PurchaseIntentWinningCategoryList
      GetPurchaseIntentWinningCategories();

  void MaybeServeAdNotification(
      const bool should_serve);
  void ServeAdNotificationIfReady();
  void ServeAdNotificationFromCategories(
      const classification::CategoryList& categories);
  void OnServeAdNotificationFromCategories(
      const Result result,
      const classification::CategoryList& categories,
      const CreativeAdNotificationList& ads);
  bool ServeAdNotificationFromParentCategories(
      const classification::CategoryList& categories);
  void ServeUntargetedAdNotification();
  void OnServeUntargetedAdNotification(
      const Result result,
      const classification::CategoryList& categories,
      const CreativeAdNotificationList& ads);
  classification::CategoryList GetCategoriesToServeAd();
  void ServeAdNotificationWithPacing(
      const CreativeAdNotificationList& ads);
  void SuccessfullyServedAd();
  void FailedToServeAdNotification(
      const std::string& reason);

  CreativeAdNotificationList GetEligibleAds(
      const CreativeAdNotificationList& ads);
  CreativeAdNotificationList GetUnseenAdsAndRoundRobinIfNeeded(
      const CreativeAdNotificationList& ads) const;
  CreativeAdNotificationList GetUnseenAds(
      const CreativeAdNotificationList& ads) const;
  CreativeAdNotificationList GetAdsForUnseenAdvertisers(
      const CreativeAdNotificationList& ads) const;

  bool IsAdNotificationValid(
      const CreativeAdNotificationInfo& info);
  bool ShowAdNotification(
      const CreativeAdNotificationInfo& info);
  bool IsAllowedToServeAdNotifications();

  Timer deliver_ad_notification_timer_;
  void StartDeliveringAdNotifications();
  void StartDeliveringAdNotificationsAfterSeconds(
      const uint64_t seconds);
  void DeliverAdNotification();

  #if defined(OS_ANDROID)
  void RemoveAllAdNotificationsAfterReboot();
  void RemoveAllAdNotificationsAfterUpdate();
  #endif

  const AdNotificationInfo& get_last_shown_ad_notification() const;
  void set_last_shown_ad_notification(
      const AdNotificationInfo& info);

  void AppendAdNotificationToHistory(
      const AdNotificationInfo& info,
      const ConfirmationType& confirmation_type);

  // Ad rewards
  void UpdateAdRewards(
      const bool should_reconcile) override;

  // Transaction history
  void GetTransactionHistory(
      GetTransactionHistoryCallback callback) override;
  TransactionList GetTransactions(
      const uint64_t from_timestamp_in_seconds,
      const uint64_t to_timestamp_in_seconds);
  TransactionList GetUnredeemedTransactions();

  // Wallet
  WalletInfo get_wallet() const;

 private:
  bool is_initialized_ = false;

  int32_t active_tab_id_ = 0;
  std::string active_tab_url_;
  std::string previous_tab_url_;

  AdNotificationInfo last_shown_ad_notification_;
  CreativeAdNotificationInfo last_shown_creative_ad_notification_;
  Timer sustain_ad_notification_interaction_timer_;
  std::set<int32_t> sustained_ad_notifications_;
  std::set<int32_t> sustaining_ad_notifications_;
  void MaybeStartSustainingAdNotificationInteraction(
      const int32_t tab_id,
      const std::string& url);
  void SustainAdNotificationInteractionIfNeeded(
      const int32_t tab_id,
      const std::string& url);

  std::vector<std::unique_ptr<PermissionRule>> CreatePermissionRules() const;
  std::vector<std::unique_ptr<ExclusionRule>> CreateExclusionRules() const;

  WalletInfo wallet_;

  AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<AdConversions> ad_conversions_;
  std::unique_ptr<AdNotifications> ad_notifications_;
  std::unique_ptr<AdRewards> ad_rewards_;
  std::unique_ptr<Bundle> bundle_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<Confirmations> confirmations_;
  std::unique_ptr<database::Initialize> database_;
  std::unique_ptr<GetCatalog> get_catalog_;
  std::unique_ptr<classification::PageClassifier> page_classifier_;
  std::unique_ptr<classification::PurchaseIntentClassifier>
      purchase_intent_classifier_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RedeemUnblindedToken> redeem_unblinded_token_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;

  // RedeemUnblindedTokenDelegate implementation
  void OnDidRedeemUnblindedToken(
      const ConfirmationInfo& confirmation) override;
  void OnFailedToRedeemUnblindedToken(
      const ConfirmationInfo& confirmation) override;

  // RedeemUnblindedPaymentTokensDelegate implementation
  void OnDidRedeemUnblindedPaymentTokens() override;
  void OnFailedToRedeemUnblindedPaymentTokens() override;
  void OnDidRetryRedeemingUnblindedPaymentTokens() override;

  // RefillUnblindedTokensDelegate implementation
  void OnDidRefillUnblindedTokens() override;
  void OnFailedToRefillUnblindedTokens() override;
  void OnDidRetryRefillingUnblindedTokens() override;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ADS_IMPL_H_
