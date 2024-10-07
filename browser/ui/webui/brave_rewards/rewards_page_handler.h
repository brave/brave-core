/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_rewards/common/mojom/rewards_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace brave_adaptive_captcha {
class BraveAdaptiveCaptchaService;
}

namespace brave_ads {
class AdsService;
}

namespace brave_rewards {

class RewardsService;

// The WebUI handler for messages originating from the Rewards page.
class RewardsPageHandler : public mojom::RewardsPageHandler {
 public:
  // An optional helper that can be supplied by WebUI controller to allow
  // the WebUI application to execute bubble actions.
  class BubbleDelegate {
   public:
    virtual ~BubbleDelegate() = default;
    virtual void ShowUI() = 0;
    virtual void OpenTab(const std::string& url) = 0;
    virtual std::string GetPublisherIdForActiveTab() = 0;
  };

  RewardsPageHandler(
      mojo::PendingRemote<mojom::RewardsPage> page,
      mojo::PendingReceiver<mojom::RewardsPageHandler> receiver,
      std::unique_ptr<BubbleDelegate> bubble_delegate,
      RewardsService* rewards_service,
      brave_ads::AdsService* ads_service,
      brave_adaptive_captcha::BraveAdaptiveCaptchaService* captcha_service,
      PrefService* prefs);

  ~RewardsPageHandler() override;

  // mojom::RewardsPageHandler:
  void OnPageReady() override;
  void OpenTab(const std::string& url) override;
  void GetPluralString(const std::string& key,
                       int32_t count,
                       GetPluralStringCallback callback) override;
  void GetRewardsParameters(GetRewardsParametersCallback callback) override;
  void GetAvailableCountries(GetAvailableCountriesCallback callback) override;
  void GetRewardsPaymentId(GetRewardsPaymentIdCallback callback) override;
  void GetCountryCode(GetCountryCodeCallback callback) override;
  void GetExternalWallet(GetExternalWalletCallback callback) override;
  void GetExternalWalletProviders(
      GetExternalWalletProvidersCallback callback) override;
  void GetAvailableBalance(GetAvailableBalanceCallback callback) override;
  void GetTermsOfServiceUpdateRequired(
      GetTermsOfServiceUpdateRequiredCallback callback) override;
  void AcceptTermsOfServiceUpdate(
      AcceptTermsOfServiceUpdateCallback callback) override;
  void GetSelfCustodyInviteDismissed(
      GetSelfCustodyInviteDismissedCallback callback) override;
  void DismissSelfCustodyInvite(
      DismissSelfCustodyInviteCallback callback) override;
  void GetPublisherForActiveTab(
      GetPublisherForActiveTabCallback callback) override;
  void GetPublisherBannerForActiveTab(
      GetPublisherBannerForActiveTabCallback callback) override;
  void GetRecurringContributions(
      GetRecurringContributionsCallback callback) override;
  void RemoveRecurringContribution(
      const std::string& creator_id,
      RemoveRecurringContributionCallback callback) override;
  void GetAutoContributeSettings(
      GetAutoContributeSettingsCallback callback) override;
  void GetAutoContributeSites(GetAutoContributeSitesCallback callback) override;
  void SetAutoContributeEnabled(
      bool enabled,
      SetAutoContributeEnabledCallback callback) override;
  void SetAutoContributeAmount(
      double amount,
      SetAutoContributeAmountCallback callback) override;
  void RemoveAutoContributeSite(
      const std::string& creator_id,
      RemoveAutoContributeSiteCallback callback) override;
  void GetAdsSettings(GetAdsSettingsCallback callback) override;
  void GetAdsStatement(GetAdsStatementCallback callback) override;
  void GetAdsHistory(GetAdsHistoryCallback callback) override;
  void SetAdTypeEnabled(brave_ads::mojom::AdType ad_type,
                        bool enabled,
                        SetAdTypeEnabledCallback callback) override;
  void SetNotificationAdsPerHour(
      int32_t ads_per_hour,
      SetNotificationAdsPerHourCallback callback) override;
  void SetAdsSubdivision(const std::string& subdivision,
                         SetAdsSubdivisionCallback callback) override;
  void ToggleAdLike(const std::string& history_item,
                    ToggleAdLikeCallback callback) override;
  void ToggleAdDislike(const std::string& history_item,
                       ToggleAdDislikeCallback callback) override;
  void ToggleAdInappropriate(const std::string& history_item,
                             ToggleAdInappropriateCallback callback) override;
  void GetRewardsNotifications(
      GetRewardsNotificationsCallback callback) override;
  void ClearRewardsNotification(
      const std::string& id,
      ClearRewardsNotificationCallback callback) override;
  void EnableRewards(const std::string& country_code,
                     EnableRewardsCallback callback) override;
  void BeginExternalWalletLogin(
      const std::string& provider,
      BeginExternalWalletLoginCallback callback) override;
  void ConnectExternalWallet(
      const std::string& provider,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback callback) override;
  void SendContribution(const std::string& creator_id,
                        double amount,
                        bool recurring,
                        SendContributionCallback callback) override;
  void GetCaptchaInfo(GetCaptchaInfoCallback callback) override;
  void OnCaptchaResult(bool success, OnCaptchaResultCallback callback) override;
  void ResetRewards(ResetRewardsCallback callback) override;

 private:
  class UpdateObserver;
  enum class UpdateSource { kAds, kRewards };
  void OnUpdate(UpdateSource update_source);

  mojo::Receiver<mojom::RewardsPageHandler> receiver_;
  mojo::Remote<mojom::RewardsPage> page_;
  std::unique_ptr<BubbleDelegate> bubble_delegate_;
  std::unique_ptr<UpdateObserver> update_observer_;
  raw_ptr<RewardsService> rewards_service_ = nullptr;
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;
  raw_ptr<brave_adaptive_captcha::BraveAdaptiveCaptchaService>
      captcha_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_HANDLER_H_
