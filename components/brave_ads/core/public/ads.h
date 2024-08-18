/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_H_

#include <memory>
#include <optional>
#include <string>

#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_observer_interface.h"
#include "brave/components/brave_ads/core/public/export.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

class AdsClient;
struct NotificationAdInfo;

class ADS_EXPORT Ads {
 public:
  Ads() = default;

  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;

  Ads(Ads&&) noexcept = delete;
  Ads& operator=(Ads&&) noexcept = delete;

  virtual ~Ads() = default;

  static Ads* CreateInstance(AdsClient* ads_client);

  virtual void AddBatAdsObserver(
      std::unique_ptr<AdsObserverInterface> observer) = 0;

  virtual void SetSysInfo(mojom::SysInfoPtr sys_info) = 0;

  virtual void SetBuildChannel(mojom::BuildChannelInfoPtr build_channel) = 0;

  virtual void SetFlags(mojom::FlagsPtr flags) = 0;

  // Called to initialize ads for the specified `mojom::WalletInfoPtr`. `wallet`
  // can be nullptr if there is no wallet. The callback takes one argument -
  // `bool` is set to `true` if successful otherwise `false`.
  virtual void Initialize(mojom::WalletInfoPtr wallet,
                          InitializeCallback callback) = 0;

  // Called to get diagnostics to help identify issues. The callback takes one
  // argument - `base::Value::List` containing info of the obtained diagnostics.
  virtual void GetDiagnostics(GetDiagnosticsCallback callback) = 0;

  // Called to get the statement of accounts. The callback takes one argument -
  // `mojom::StatementInfo` containing info of the obtained statement of
  // accounts.
  virtual void GetStatementOfAccounts(
      GetStatementOfAccountsCallback callback) = 0;

  // Called to serve an inline content ad for the specified `dimensions`. The
  // callback takes two arguments - `std::string` containing the dimensions and
  // `InlineContentAdInfo` containing the info for the ad.
  virtual void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) = 0;

  // Called when a user views or interacts with an inline content ad to trigger
  // an `event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random UUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed impression event should be used for all other
  // events for the same ad placement. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`. Must be called before the
  // `mojom::InlineContentAdEventType::target_url` landing page is opened.
  virtual void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType event_type,
      TriggerAdEventCallback callback) = 0;

  // Called to serve a new tab page ad. The callback takes one argument -
  // `NewTabPageAdInfo` containing the info for the ad.
  virtual void MaybeServeNewTabPageAd(
      MaybeServeNewTabPageAdCallback callback) = 0;

  // Called when a user views or interacts with a new tab page ad to trigger an
  // `event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random UUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed impression event should be used for all other
  // events for the same ad placement. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`. Must be called before the
  // `mojom::NewTabPageAdEventType::target_url` landing page is opened.
  virtual void TriggerNewTabPageAdEvent(const std::string& placement_id,
                                        const std::string& creative_instance_id,
                                        mojom::NewTabPageAdEventType event_type,
                                        TriggerAdEventCallback callback) = 0;

  // Called to get the notification ad specified by `placement_id`. Returns
  // `NotificationAdInfo` containing the info of the ad.
  virtual std::optional<NotificationAdInfo> MaybeGetNotificationAd(
      const std::string& placement_id) = 0;

  // Called when a user views or interacts with a notification ad or the ad
  // notification times out to trigger an `event_type` event for the specified
  // `placement_id`. `placement_id` should be a 128-bit random UUID in the form
  // of version 4. See RFC 4122, section 4.4. The same `placement_id` generated
  // for the viewed impression event should be used for all other events for the
  // same ad placement. The callback takes one argument - `bool` is set to
  // `true` if successful otherwise `false`. Must be called before the
  // `mojom::NotificationAdEventType::target_url` landing page is opened.
  virtual void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType event_type,
      TriggerAdEventCallback callback) = 0;

  // Called when a user views or interacts with a promoted content ad to trigger
  // an `event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random UUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed impression event should be used for all other
  // events for the same ad placement. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`. Must be called before the
  // `mojom::PromotedContentAdEventType::target_url` landing page is opened.
  virtual void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType event_type,
      TriggerAdEventCallback callback) = 0;

  // Called when a user views or interacts with a search result ad to trigger an
  // `event_type` event for the ad specified in `mojom_creative_ad`. The
  // callback takes one argument - `bool` is set to `true` if successful
  // otherwise `false`. Must be called before the
  // `mojom::CreativeSearchResultAdInfo::target_url` landing page is opened.
  virtual void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType event_type,
      TriggerAdEventCallback callback) = 0;

  // Called to purge orphaned served ad events for the specified `ad_type`
  // before calling `MaybeServe*Ad`. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void PurgeOrphanedAdEventsForType(
      mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) = 0;

  // Called to get ad history for the given date range in descending order. The
  // callback takes one argument - `base::Value::List` containing info of the
  // obtained ad history.
  virtual void GetAdHistory(base::Time from_time,
                            base::Time to_time,
                            GetAdHistoryForUICallback callback) = 0;

  // Called to like an ad. This is a toggle, so calling it again returns the
  // setting to the neutral state. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void ToggleLikeAd(const base::Value::Dict& value,
                            ToggleReactionCallback callback) = 0;

  // Called to dislike an ad. This is a toggle, so calling it again returns the
  // setting to the neutral state. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void ToggleDislikeAd(const base::Value::Dict& value,
                               ToggleReactionCallback callback) = 0;

  // Called to like a category. This is a toggle, so calling it again returns
  // the setting to the neutral state. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`.
  virtual void ToggleLikeSegment(const base::Value::Dict& value,
                                 ToggleReactionCallback callback) = 0;

  // Called to dislike a category. This is a toggle, so calling it again
  // returns the setting to the neutral state. The callback takes one argument -
  // `bool` is set to `true` if successful otherwise `false`.
  virtual void ToggleDislikeSegment(const base::Value::Dict& value,
                                    ToggleReactionCallback callback) = 0;

  // Called to save an ad for later viewing. This is a toggle, so calling it
  // again removes the ad from the saved list. The callback takes one argument -
  // `bool` is set to `true` if successful otherwise `false`.
  virtual void ToggleSaveAd(const base::Value::Dict& value,
                            ToggleReactionCallback callback) = 0;

  // Called to mark an ad as inappropriate. This is a toggle, so calling it
  // again unmarks the ad. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void ToggleMarkAdAsInappropriate(const base::Value::Dict& value,
                                           ToggleReactionCallback callback) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_H_
