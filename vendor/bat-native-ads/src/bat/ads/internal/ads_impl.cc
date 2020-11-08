/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <utility>

#include "bat/ads/ad_history_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/statement.h"
#include "bat/ads/internal/account/wallet.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/ad_server/ad_server.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ad_targeting/behavioral/purchase_intent_classifier/purchase_intent_classifier.h"
#include "bat/ads/internal/ad_targeting/behavioral/purchase_intent_classifier/purchase_intent_classifier_user_models.h"
#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier.h"
#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier_user_models.h"
#include "bat/ads/internal/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_transfer/ad_transfer.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/bundle/bundle.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/conversions/conversions.h"
#include "bat/ads/internal/database/database_initialize.h"
#include "bat/ads/internal/features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/tabs/tabs.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/pref_names.h"

namespace ads {

using std::placeholders::_1;

namespace {
const int kIdleThresholdInSeconds = 15;
}  // namespace

AdsImpl::AdsImpl(
    AdsClient* ads_client)
    : ads_client_(ads_client),
      ads_history_(std::make_unique<AdsHistory>(this)),
      ad_notification_(std::make_unique<AdNotification>(this)),
      ad_notifications_(std::make_unique<AdNotifications>(this)),
      ad_rewards_(std::make_unique<AdRewards>(this)),
      ad_server_(std::make_unique<AdServer>(this)),
      ad_notification_serving_(std::make_unique<
          ad_notifications::AdServing>(this)),
      ad_targeting_(std::make_unique<AdTargeting>(this)),
      ad_transfer_(std::make_unique<AdTransfer>(this)),
      bundle_(std::make_unique<Bundle>(this)),
      client_(std::make_unique<Client>(this)),
      confirmations_(std::make_unique<Confirmations>(this)),
      conversions_(std::make_unique<Conversions>(this)),
      database_(std::make_unique<database::Initialize>(this)),
      new_tab_page_ad_(std::make_unique<NewTabPageAd>(this)),
      purchase_intent_classifier_(std::make_unique<
          ad_targeting::behavioral::PurchaseIntentClassifier>(this)),
      page_classifier_(std::make_unique<
          ad_targeting::contextual::PageClassifier>(this)),
      subdivision_targeting_(std::make_unique<
          ad_targeting::geographic::SubdivisionTargeting>(this)),
      redeem_unblinded_payment_tokens_(std::make_unique<
          RedeemUnblindedPaymentTokens>(this)),
      redeem_unblinded_token_(std::make_unique<RedeemUnblindedToken>(this)),
      refill_unblinded_tokens_(std::make_unique<RefillUnblindedTokens>(this)),
      tabs_(std::make_unique<Tabs>(this)),
      user_activity_(std::make_unique<UserActivity>()),
      wallet_(std::make_unique<Wallet>(this)) {
  set_ads_client_for_logging(ads_client_);

  redeem_unblinded_token_->set_delegate(this);
  redeem_unblinded_payment_tokens_->set_delegate(this);
  refill_unblinded_tokens_->set_delegate(this);
}

AdsImpl::~AdsImpl() = default;

bool AdsImpl::IsInitialized() {
  if (!is_initialized_ || !ads_client_->GetBooleanPref(prefs::kEnabled)) {
    return false;
  }

  return true;
}

bool AdsImpl::IsForeground() const {
  return is_foreground_;
}

void AdsImpl::Initialize(
    InitializeCallback callback) {
  BLOG(1, "Initializing ads");

  if (IsInitialized()) {
    BLOG(1, "Already initialized ads");
    callback(FAILED);
    return;
  }

  const auto initialize_step_2_callback =
      std::bind(&AdsImpl::InitializeStep2, this, _1, std::move(callback));
  database_->CreateOrOpen(initialize_step_2_callback);
}

void AdsImpl::Shutdown(
    ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");

    callback(FAILED);
    return;
  }

  ad_notifications_->RemoveAll(true);

  callback(SUCCESS);
}

void AdsImpl::ChangeLocale(
    const std::string& locale) {
  subdivision_targeting_->MaybeFetchForLocale(locale);
  page_classifier_->LoadUserModelForLocale(locale);
  purchase_intent_classifier_->LoadUserModelForLocale(locale);
}

void AdsImpl::OnAdsSubdivisionTargetingCodeHasChanged() {
  subdivision_targeting_->MaybeFetchForCurrentLocale();
}

void AdsImpl::OnPageLoaded(
    const int32_t tab_id,
    const std::string& original_url,
    const std::string& url,
    const std::string& content) {
  DCHECK(!original_url.empty());
  DCHECK(!url.empty());

  if (!IsInitialized()) {
    return;
  }

  ad_transfer_->MaybeTransferAd(tab_id, original_url);

  conversions_->MaybeConvert(url);

  purchase_intent_classifier_->MaybeExtractIntentSignal(url);

  page_classifier_->MaybeClassifyPage(url, content);
}

void AdsImpl::OnIdle() {
  BLOG(1, "Browser state changed to idle");
}

void AdsImpl::OnUnIdle() {
  if (!IsInitialized()) {
    return;
  }

  BLOG(1, "Browser state changed to unidle");

  MaybeServeAdNotification();
}

void AdsImpl::OnForeground() {
  is_foreground_ = true;

  BLOG(1, "Browser window did become active");

  user_activity_->RecordActivityForType(
      UserActivityType::kBrowserWindowDidBecomeActive);

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::OnBackground() {
  is_foreground_ = false;

  BLOG(1, "Browser window did enter background");

  user_activity_->RecordActivityForType(
      UserActivityType::kBrowserWindowDidEnterBackground);

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::OnMediaPlaying(
    const int32_t tab_id) {
  tabs_->OnMediaPlaying(tab_id);
}

void AdsImpl::OnMediaStopped(
    const int32_t tab_id) {
  tabs_->OnMediaStopped(tab_id);
}

void AdsImpl::OnTabUpdated(
    const int32_t tab_id,
    const std::string& url,
    const bool is_active,
    const bool is_browser_active,
    const bool is_incognito) {
  const bool is_visible = is_active && is_browser_active;
  tabs_->OnUpdated(tab_id, url, is_visible, is_incognito);
}

void AdsImpl::OnTabClosed(
    const int32_t tab_id) {
  tabs_->OnClosed(tab_id);
}

void AdsImpl::OnWalletUpdated(
    const std::string& id,
    const std::string& seed) {
  if (!wallet_->Set(id, seed)) {
    BLOG(0, "Failed to update wallet");
    return;
  }

  BLOG(1, "Successfully updated wallet");
}

void AdsImpl::OnUserModelUpdated(
    const std::string& id) {
  if (kPageClassificationUserModelIds.find(id) !=
      kPageClassificationUserModelIds.end()) {
    page_classifier_->LoadUserModelForId(id);
  } else if (kPurchaseIntentUserModelIds.find(id) !=
      kPurchaseIntentUserModelIds.end()) {
    purchase_intent_classifier_->LoadUserModelForId(id);
  } else {
    BLOG(0, "Unknown " << id << " user model");
  }
}

bool AdsImpl::GetAdNotification(
    const std::string& uuid,
    AdNotificationInfo* notification) {
  return ad_notifications_->Get(uuid, notification);
}

void AdsImpl::OnAdNotificationEvent(
    const std::string& uuid,
    const AdNotificationEventType event_type) {
  ad_notification_->Trigger(uuid, event_type);
}

void AdsImpl::OnNewTabPageAdEvent(
    const std::string& wallpaper_id,
    const std::string& creative_instance_id,
    const NewTabPageAdEventType event_type) {
  new_tab_page_ad_->Trigger(wallpaper_id, creative_instance_id, event_type);
}

void AdsImpl::RemoveAllHistory(
    RemoveAllHistoryCallback callback) {
  client_->RemoveAllHistory();

  callback(SUCCESS);
}

void AdsImpl::ReconcileAdRewards() {
  if (!IsInitialized()) {
    return;
  }

  const WalletInfo wallet = wallet_->Get();
  ad_rewards_->MaybeReconcile(wallet);
}

AdsHistoryInfo AdsImpl::GetAdsHistory(
    const AdsHistoryInfo::FilterType filter_type,
    const AdsHistoryInfo::SortType sort_type,
    const uint64_t from_timestamp,
    const uint64_t to_timestamp) {
  return ads_history_->Get(filter_type, sort_type,
      from_timestamp, to_timestamp);
}

void AdsImpl::GetTransactionHistory(
    GetTransactionHistoryCallback callback) {
  StatementInfo statement_of_account;

  if (!IsInitialized()) {
    callback(/* success */ false, statement_of_account);
    return;
  }

  const int64_t to_timestamp =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());

  Statement statement(this);
  statement_of_account = statement.Get(0, to_timestamp);

  callback(/* success */ true, statement_of_account);
}

AdContentInfo::LikeAction AdsImpl::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentInfo::LikeAction& action) {
  auto like_action =
      client_->ToggleAdThumbUp(creative_instance_id, creative_set_id, action);
  if (like_action == AdContentInfo::LikeAction::kThumbsUp) {
    confirmations_->ConfirmAd(creative_instance_id, ConfirmationType::kUpvoted);
  }

  return like_action;
}

AdContentInfo::LikeAction AdsImpl::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentInfo::LikeAction& action) {
  auto like_action =
      client_->ToggleAdThumbDown(creative_instance_id, creative_set_id, action);
  if (like_action == AdContentInfo::LikeAction::kThumbsDown) {
    confirmations_->ConfirmAd(creative_instance_id,
        ConfirmationType::kDownvoted);
  }

  return like_action;
}

CategoryContentInfo::OptAction AdsImpl::ToggleAdOptInAction(
    const std::string& category,
    const CategoryContentInfo::OptAction& action) {
  return client_->ToggleAdOptInAction(category, action);
}

CategoryContentInfo::OptAction AdsImpl::ToggleAdOptOutAction(
    const std::string& category,
    const CategoryContentInfo::OptAction& action) {
  return client_->ToggleAdOptOutAction(category, action);
}

bool AdsImpl::ToggleSaveAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool saved) {
  return client_->ToggleSaveAd(creative_instance_id, creative_set_id, saved);
}

bool AdsImpl::ToggleFlagAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool flagged) {
  auto flag_ad =
      client_->ToggleFlagAd(creative_instance_id, creative_set_id, flagged);
  if (flag_ad) {
    confirmations_->ConfirmAd(creative_instance_id, ConfirmationType::kFlagged);
  }

  return flag_ad;
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::InitializeStep2(
    const Result result,
    InitializeCallback callback) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to initialize database: " << database_->get_last_message());
    callback(FAILED);
    return;
  }

  const auto initialize_step_3_callback =
      std::bind(&AdsImpl::InitializeStep3, this, _1, std::move(callback));
  client_->Initialize(initialize_step_3_callback);
}

void AdsImpl::InitializeStep3(
    const Result result,
    InitializeCallback callback) {
  if (result != SUCCESS) {
    callback(FAILED);
    return;
  }

  const auto initialize_step_4_callback =
      std::bind(&AdsImpl::InitializeStep4, this, _1, std::move(callback));
  confirmations_->Initialize(initialize_step_4_callback);
}

void AdsImpl::InitializeStep4(
    const Result result,
    InitializeCallback callback) {
  if (result != SUCCESS) {
    callback(FAILED);
    return;
  }

  const auto initialize_step_5_callback =
      std::bind(&AdsImpl::InitializeStep5, this, _1, std::move(callback));
  ad_notifications_->Initialize(initialize_step_5_callback);
}

void AdsImpl::InitializeStep5(
    const Result result,
    InitializeCallback callback) {
  if (result != SUCCESS) {
    callback(FAILED);
    return;
  }

  const auto initialize_step_6_callback =
      std::bind(&AdsImpl::InitializeStep6, this, _1, std::move(callback));
  conversions_->Initialize(initialize_step_6_callback);
}

void AdsImpl::InitializeStep6(
    const Result result,
    InitializeCallback callback) {
  if (result != SUCCESS) {
    callback(FAILED);
    return;
  }

  is_initialized_ = true;

  BLOG(1, "Successfully initialized ads");

  is_foreground_ = ads_client_->IsForeground();

  ads_client_->SetIntegerPref(prefs::kIdleThreshold, kIdleThresholdInSeconds);

  callback(SUCCESS);

  ReconcileAdRewards();

  subdivision_targeting_->MaybeFetchForCurrentLocale();

  const WalletInfo wallet = wallet_->Get();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  PurgeExpiredAdEvents();

  conversions_->StartTimerIfReady();

#if defined(OS_ANDROID)
    // Ad notifications do not sustain a reboot or update, so we should remove
    // orphaned ad notifications
    ad_notifications_->RemoveAllAfterReboot();
    ad_notifications_->RemoveAllAfterUpdate();
#endif

  features::LogPageProbabilitiesStudy();

  MaybeServeAdNotificationsAtRegularIntervals();

  ad_server_->MaybeFetch();
}

void AdsImpl::PurgeExpiredAdEvents() {
  AdEvents ad_events(this);
  ad_events.PurgeExpired([](
      const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to purge expired ad events");
      return;
    }

    BLOG(6, "Successfully purged expired ad events");
  });
}

void AdsImpl::MaybeServeAdNotification() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  ad_notification_serving_->MaybeServe();
}

void AdsImpl::MaybeServeAdNotificationsAtRegularIntervals() {
  if (!PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  if (is_foreground_ || ads_client_->CanShowBackgroundNotifications()) {
    ad_notification_serving_->ServeAtRegularIntervals();
  } else {
    ad_notification_serving_->StopServing();
  }
}

void AdsImpl::OnDidRedeemUnblindedToken(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Successfully redeemed unblinded token with confirmation id "
      << confirmation.id << ", creative instance id "
          << confirmation.creative_instance_id << " and "
              << std::string(confirmation.type));
}

void AdsImpl::OnFailedToRedeemUnblindedToken(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Failed to redeem unblinded token with confirmation id "
      << confirmation.id << ", creative instance id "
          <<  confirmation.creative_instance_id << " and "
              << std::string(confirmation.type));
}

void AdsImpl::OnDidRedeemUnblindedPaymentTokens() {
  BLOG(1, "Successfully redeemed unblinded payment tokens");

  ReconcileAdRewards();
}

void AdsImpl::OnFailedToRedeemUnblindedPaymentTokens() {
  BLOG(1, "Failed to redeem unblinded payment tokens");
}

void AdsImpl::OnDidRetryRedeemingUnblindedPaymentTokens() {
  BLOG(1, "Retry redeeming unblinded payment tokens");
}

void AdsImpl::OnDidRefillUnblindedTokens() {
  BLOG(1, "Successfully refilled unblinded tokens");
}

void AdsImpl::OnFailedToRefillUnblindedTokens() {
  BLOG(1, "Failed to refill unblinded tokens");
}

void AdsImpl::OnDidRetryRefillingUnblindedTokens() {
  BLOG(1, "Retry refilling unblinded tokens");
}

}  // namespace ads
