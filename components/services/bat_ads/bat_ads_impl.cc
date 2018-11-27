/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include "bat/ads/ads.h"
#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

namespace bat_ads {

namespace {

ads::NotificationResultInfoResultType ToNotificationResultInfoResultType(
    const std::string& result_type) {
  // TODO(bridiver) - this goes away with json serialization
  if (result_type == "clicked") {
    return ads::NotificationResultInfoResultType::CLICKED;
  } else if (result_type == "dismissed") {
    return ads::NotificationResultInfoResultType::DISMISSED;
  } else if (result_type == "timeout") {
    return ads::NotificationResultInfoResultType::TIMEOUT;
  } else {
    NOTREACHED();
    return ads::NotificationResultInfoResultType::DISMISSED;
  }
}

}

BatAdsImpl::BatAdsImpl(mojom::BatAdsClientAssociatedPtrInfo client_info)
    : bat_ads_client_mojo_proxy_(
          new BatAdsClientMojoBridge(std::move(client_info))),
      ads_(ads::Ads::CreateInstance(bat_ads_client_mojo_proxy_.get())) {}

BatAdsImpl::~BatAdsImpl() {}

void BatAdsImpl::Initialize(InitializeCallback callback) {
  ads_->Initialize();
  std::move(callback).Run();
}

void BatAdsImpl::ClassifyPage(const std::string& url,
                              const std::string& page,
                              ClassifyPageCallback callback) {
  ads_->ClassifyPage(url, page);
  std::move(callback).Run();
}

void BatAdsImpl::TabClosed(int32_t tab_id, TabClosedCallback callback) {
  ads_->TabClosed(tab_id);
  std::move(callback).Run();
}

void BatAdsImpl::OnTimer(uint32_t timer_id, OnTimerCallback callback) {
  ads_->OnTimer(timer_id);
  std::move(callback).Run();
}

void BatAdsImpl::OnUnIdle(OnUnIdleCallback callback) {
  ads_->OnUnIdle();
  std::move(callback).Run();
}

void BatAdsImpl::OnIdle(OnIdleCallback callback) {
  ads_->OnIdle();
  std::move(callback).Run();
}

void BatAdsImpl::SaveCachedInfo(SaveCachedInfoCallback callback) {
  ads_->SaveCachedInfo();
  std::move(callback).Run();
}

void BatAdsImpl::OnForeground(OnForegroundCallback callback) {
  ads_->OnForeground();
  std::move(callback).Run();
}

void BatAdsImpl::OnBackground(OnBackgroundCallback callback) {
  ads_->OnBackground();
  std::move(callback).Run();
}

void BatAdsImpl::OnMediaPlaying(int32_t tab_id,
                                OnMediaPlayingCallback callback) {
  ads_->OnMediaPlaying(tab_id);
  std::move(callback).Run();
}

void BatAdsImpl::OnMediaStopped(int32_t tab_id,
                                OnMediaStoppedCallback callback) {
  ads_->OnMediaStopped(tab_id);
  std::move(callback).Run();
}

void BatAdsImpl::TabUpdated(int32_t tab_id,
                const std::string& url,
                bool is_active,
                bool is_incognito,
                TabUpdatedCallback callback) {
  ads_->TabUpdated(tab_id, url, is_active, is_incognito);
  std::move(callback).Run();
}

void BatAdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  ads_->RemoveAllHistory();
  std::move(callback).Run();
}

void BatAdsImpl::ServeSampleAd(ServeSampleAdCallback callback) {
  ads_->ServeSampleAd();
  std::move(callback).Run();
}

void BatAdsImpl::GenerateAdReportingNotificationShownEvent(
      const std::string& notification_info,
      GenerateAdReportingNotificationShownEventCallback callback) {
  auto info = std::make_unique<ads::NotificationInfo>();
  if (info->FromJson(notification_info)) {
    ads_->GenerateAdReportingNotificationShownEvent(*info);
  } else {
    // TODO(bridiver) ?
  }
  std::move(callback).Run();
}

void BatAdsImpl::GenerateAdReportingNotificationResultEvent(
      const std::string& notification_info,
      const std::string& result_type,
      GenerateAdReportingNotificationResultEventCallback callback) {
  auto info = std::make_unique<ads::NotificationInfo>();
  if (info->FromJson(notification_info)) {
    ads_->GenerateAdReportingNotificationResultEvent(
        *info,
        ToNotificationResultInfoResultType(result_type));
  } else {
    // TODO(bridiver) ?
  }
  std::move(callback).Run();
}

}  // namespace bat_ads
