/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_ads::test {

// Minimal in-process implementation of `bat_ads::mojom::BatAds`. Only
// `Initialize` and `Shutdown` need real behaviour for the tests in this suite;
// all other methods are no-ops.
class FakeBatAds : public bat_ads::mojom::BatAds {
 public:
  // `initialize_callback` is called when `Initialize` succeeds.
  FakeBatAds(base::RepeatingClosure initialize_callback,
             bool simulate_initialization_failure);

  FakeBatAds(const FakeBatAds&) = delete;
  FakeBatAds& operator=(const FakeBatAds&) = delete;

  ~FakeBatAds() override;

  void BindReceiver(mojo::PendingAssociatedReceiver<bat_ads::mojom::BatAds>
                        bat_ads_pending_associated_receiver);

  // bat_ads::mojom::BatAds:
  void AddBatAdsObserver(mojo::PendingRemote<bat_ads::mojom::BatAdsObserver>
                         /*bat_ads_observer_pending_remote*/) override {}
  void SetSysInfo(brave_ads::mojom::SysInfoPtr /*mojom_sys_info*/) override {}
  void SetBuildChannel(
      brave_ads::mojom::BuildChannelInfoPtr /*mojom_build_channel*/) override {}
  void SetCommandLineSwitches(brave_ads::mojom::CommandLineSwitchesPtr
                              /*mojom_command_line_switches*/) override {}
  void SetContentSettings(brave_ads::mojom::ContentSettingsPtr
                          /*mojom_content_settings*/) override {}
  void Initialize(brave_ads::mojom::WalletInfoPtr /*mojom_wallet*/,
                  InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;
  void GetInternals(GetInternalsCallback callback) override;
  void GetDiagnostics(GetDiagnosticsCallback callback) override;
  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;
  void ParseAndSaveNewTabPageAds(
      base::DictValue /*value*/,
      ParseAndSaveNewTabPageAdsCallback callback) override;
  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& /*placement_id*/,
      const std::string& /*creative_instance_id*/,
      brave_ads::mojom::NewTabPageAdMetricType /*mojom_ad_metric_type*/,
      brave_ads::mojom::NewTabPageAdEventType /*mojom_ad_event_type*/,
      TriggerNewTabPageAdEventCallback callback) override;
  void MaybeGetNotificationAd(const std::string& /*placement_id*/,
                              MaybeGetNotificationAdCallback callback) override;
  void TriggerNotificationAdEvent(
      const std::string& /*placement_id*/,
      brave_ads::mojom::NotificationAdEventType /*mojom_ad_event_type*/,
      TriggerNotificationAdEventCallback callback) override;
  void MaybeGetSearchResultAd(const std::string& /*placement_id*/,
                              MaybeGetSearchResultAdCallback callback) override;
  void TriggerSearchResultAdEvent(
      brave_ads::mojom::CreativeSearchResultAdInfoPtr /*mojom_creative_ad*/,
      brave_ads::mojom::SearchResultAdEventType /*mojom_ad_event_type*/,
      TriggerSearchResultAdEventCallback callback) override;
  void PurgeOrphanedAdEventsForType(
      brave_ads::mojom::AdType /*mojom_ad_type*/,
      PurgeOrphanedAdEventsForTypeCallback callback) override;
  void GetAdHistory(base::Time /*from_time*/,
                    base::Time /*to_time*/,
                    GetAdHistoryCallback callback) override;
  void ToggleLikeAd(brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
                    ToggleLikeAdCallback callback) override;
  void ToggleDislikeAd(brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
                       ToggleDislikeAdCallback callback) override;
  void ToggleLikeSegment(brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
                         ToggleLikeSegmentCallback callback) override;
  void ToggleDislikeSegment(
      brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
      ToggleDislikeSegmentCallback callback) override;
  void ToggleSaveAd(brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
                    ToggleSaveAdCallback callback) override;
  void ToggleMarkAdAsInappropriate(
      brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
      ToggleMarkAdAsInappropriateCallback callback) override;

 private:
  base::RepeatingClosure initialize_callback_;
  bool simulate_initialization_failure_;

  mojo::AssociatedReceiver<bat_ads::mojom::BatAds> bat_ads_associated_receiver_{
      this};
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_H_
