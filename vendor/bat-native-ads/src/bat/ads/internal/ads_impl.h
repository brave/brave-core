/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_IMPL_H_
#define BAT_ADS_INTERNAL_ADS_IMPL_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ads/ads.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token_delegate.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "bat/ads/mojom.h"

namespace ads {

namespace ad_notifications {
class AdServing;
}  // namesapce ad_notifications

namespace ad_targeting {

namespace contextual {
class PageClassifier;
}  // namespace contextual

namespace behavioral {
class PurchaseIntentClassifier;
}  // namespace behavioral

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

}  // namespace ad_targeting

namespace database {
class Initialize;
}  // namespace database

class AdNotification;
class AdNotificationServing;
class AdNotifications;
class AdRewards;
class AdServer;
class AdsHistory;
class AdTargeting;
class AdTransfer;
class Bundle;
class Client;
class ConfirmationType;
class Confirmations;
class Conversions;
class NewTabPageAd;
class RedeemUnblindedPaymentTokens;
class RedeemUnblindedToken;
class RefillUnblindedTokens;
class Tabs;
class UserActivity;
class Wallet;

struct AdNotificationInfo;
struct AdsHistoryInfo;
struct ConfirmationInfo;

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

  AdsClient* get_ads_client() const {
    return ads_client_;
  }

  AdNotifications* get_ad_notifications() const {
    return ad_notifications_.get();
  }

  AdRewards* get_ad_rewards() const {
    return ad_rewards_.get();
  }

  AdServer* get_ad_server() const {
    return ad_server_.get();
  }

  AdTargeting* get_ad_targeting() const {
    return ad_targeting_.get();
  }

  AdTransfer* get_ad_transfer() const {
    return ad_transfer_.get();
  }

  AdsHistory* get_ads_history() const {
    return ads_history_.get();
  }

  Bundle* get_bundle() const {
    return bundle_.get();
  }

  Client* get_client() const {
    return client_.get();
  }

  Confirmations* get_confirmations() const {
    return confirmations_.get();
  }

  Conversions* get_conversions() const {
    return conversions_.get();
  }

  ad_targeting::contextual::PageClassifier* get_page_classifier() const {
    return page_classifier_.get();
  }

  ad_targeting::behavioral::PurchaseIntentClassifier*
  get_purchase_intent_classifier() const {
    return purchase_intent_classifier_.get();
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

  ad_targeting::geographic::SubdivisionTargeting*
  get_subdivision_targeting() const {
    return subdivision_targeting_.get();
  }

  Tabs* get_tabs() const {
    return tabs_.get();
  }

  UserActivity* get_user_activity() const {
    return user_activity_.get();
  }

  Wallet* get_wallet() const {
    return wallet_.get();
  }

  bool IsInitialized();

  bool IsForeground() const;

  // Ads implementation
  void Initialize(
      InitializeCallback callback) override;

  void Shutdown(
      ShutdownCallback callback) override;

  void ChangeLocale(
      const std::string& locale) override;

  void OnAdsSubdivisionTargetingCodeHasChanged() override;

  void OnPageLoaded(
      const int32_t tab_id,
      const std::string& original_url,
      const std::string& url,
      const std::string& content) override;

  void OnIdle() override;
  void OnUnIdle() override;

  void OnForeground() override;
  void OnBackground() override;

  void OnMediaPlaying(
      const int32_t tab_id) override;
  void OnMediaStopped(
      const int32_t tab_id) override;

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

  void OnUserModelUpdated(
      const std::string& id) override;

  bool GetAdNotification(
      const std::string& uuid,
      AdNotificationInfo* info) override;
  void OnAdNotificationEvent(
      const std::string& uuid,
      const AdNotificationEventType event_type) override;

  void OnNewTabPageAdEvent(
      const std::string& wallpaper_id,
      const std::string& creative_instance_id,
      const NewTabPageAdEventType event_type) override;

  void RemoveAllHistory(
      RemoveAllHistoryCallback callback) override;

  void ReconcileAdRewards() override;

  AdsHistoryInfo GetAdsHistory(
      const AdsHistoryInfo::FilterType filter_type,
      const AdsHistoryInfo::SortType sort_type,
      const uint64_t from_timestamp,
      const uint64_t to_timestamp) override;

  void GetTransactionHistory(
      GetTransactionHistoryCallback callback) override;

  AdContentInfo::LikeAction ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction& action) override;
  AdContentInfo::LikeAction ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction& action) override;
  CategoryContentInfo::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContentInfo::OptAction& action) override;
  CategoryContentInfo::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContentInfo::OptAction& action) override;
  bool ToggleSaveAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool saved) override;
  bool ToggleFlagAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool flagged) override;

 private:
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

  void PurgeExpiredAdEvents();

  void MaybeServeAdNotification();

  void MaybeServeAdNotificationsAtRegularIntervals();

  bool is_initialized_ = false;

  bool is_foreground_ = false;

  AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<AdsHistory> ads_history_;
  std::unique_ptr<AdNotification> ad_notification_;
  std::unique_ptr<AdNotifications> ad_notifications_;
  std::unique_ptr<AdRewards> ad_rewards_;
  std::unique_ptr<AdServer> ad_server_;
  std::unique_ptr<ad_notifications::AdServing> ad_notification_serving_;
  std::unique_ptr<AdTargeting> ad_targeting_;
  std::unique_ptr<AdTransfer> ad_transfer_;
  std::unique_ptr<Bundle> bundle_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<Confirmations> confirmations_;
  std::unique_ptr<Conversions> conversions_;
  std::unique_ptr<database::Initialize> database_;
  std::unique_ptr<NewTabPageAd> new_tab_page_ad_;
  std::unique_ptr<ad_targeting::behavioral::PurchaseIntentClassifier>
      purchase_intent_classifier_;
  std::unique_ptr<ad_targeting::contextual::PageClassifier> page_classifier_;
  std::unique_ptr<ad_targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RedeemUnblindedToken> redeem_unblinded_token_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<Tabs> tabs_;
  std::unique_ptr<UserActivity> user_activity_;
  std::unique_ptr<Wallet> wallet_;

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
