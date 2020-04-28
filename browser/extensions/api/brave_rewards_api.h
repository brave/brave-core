/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/browser/balance.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/external_wallet.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/promotion.h"
#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveRewardsCreateWalletFunction : public ExtensionFunction {
 public:
  BraveRewardsCreateWalletFunction();
  DECLARE_EXTENSION_FUNCTION("braveRewards.createWallet", UNKNOWN)

 protected:
  ~BraveRewardsCreateWalletFunction() override;

  ResponseAction Run() override;
 private:
  base::WeakPtrFactory<BraveRewardsCreateWalletFunction> weak_factory_;
  void OnCreateWallet(int32_t result);
};

class BraveRewardsOpenBrowserActionUIFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.openBrowserActionUI", UNKNOWN)

 protected:
  ~BraveRewardsOpenBrowserActionUIFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsTipSiteFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.tipSite", UNKNOWN)

 protected:
  ~BraveRewardsTipSiteFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsTipTwitterUserFunction
    : public ExtensionFunction {
 public:
  BraveRewardsTipTwitterUserFunction();
  DECLARE_EXTENSION_FUNCTION("braveRewards.tipTwitterUser", UNKNOWN)

 protected:
  ~BraveRewardsTipTwitterUserFunction() override;

  ResponseAction Run() override;

 private:
  base::WeakPtrFactory<BraveRewardsTipTwitterUserFunction> weak_factory_;
  void OnTwitterPublisherInfoSaved(
      std::unique_ptr<brave_rewards::ContentSite> publisher_info);
};

class BraveRewardsTipGitHubUserFunction : public ExtensionFunction {
 public:
  BraveRewardsTipGitHubUserFunction();
  DECLARE_EXTENSION_FUNCTION("braveRewards.tipGitHubUser", UNKNOWN)

 protected:
  ~BraveRewardsTipGitHubUserFunction() override;

  ResponseAction Run() override;
 private:
  base::WeakPtrFactory<BraveRewardsTipGitHubUserFunction> weak_factory_;
  void OnGitHubPublisherInfoSaved(
      std::unique_ptr<brave_rewards::ContentSite> publisher_info);
};

class BraveRewardsTipRedditUserFunction : public ExtensionFunction {
 public:
  BraveRewardsTipRedditUserFunction();
  DECLARE_EXTENSION_FUNCTION("braveRewards.tipRedditUser", UNKNOWN)

 protected:
  ~BraveRewardsTipRedditUserFunction() override;

  ResponseAction Run() override;
 private:
  base::WeakPtrFactory<BraveRewardsTipRedditUserFunction> weak_factory_;
  void OnRedditPublisherInfoSaved(
      std::unique_ptr<brave_rewards::ContentSite> publisher_info);
};

class BraveRewardsGetPublisherDataFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherData", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherDataFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetWalletPropertiesFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getWalletProperties", UNKNOWN)

 protected:
  ~BraveRewardsGetWalletPropertiesFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetBalanceReportFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getBalanceReport", UNKNOWN)

 protected:
  ~BraveRewardsGetBalanceReportFunction() override;

  ResponseAction Run() override;

 private:
  void OnBalanceReport(
      const int32_t result,
      const brave_rewards::BalanceReport& report);
};

class BraveRewardsIncludeInAutoContributionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.includeInAutoContribution", UNKNOWN)

 protected:
  ~BraveRewardsIncludeInAutoContributionFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsFetchPromotionsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.fetchPromotions", UNKNOWN)

 protected:
  ~BraveRewardsFetchPromotionsFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsClaimPromotionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.claimPromotion", UNKNOWN)

 protected:
  ~BraveRewardsClaimPromotionFunction() override;

  ResponseAction Run() override;

 private:
  void OnClaimPromotion(
      const std::string& promotion_id,
      const int32_t result,
      const std::string& captcha_image,
      const std::string& hint,
      const std::string& captcha_id);
};

class BraveRewardsAttestPromotionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.attestPromotion", UNKNOWN)

 protected:
  ~BraveRewardsAttestPromotionFunction() override;

  ResponseAction Run() override;

 private:
  void OnAttestPromotion(
      const std::string& promotion_id,
      const int32_t result,
      std::unique_ptr<::brave_rewards::Promotion> promotion);
};

class BraveRewardsGetPendingContributionsTotalFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "braveRewards.getPendingContributionsTotal", UNKNOWN)

 protected:
  ~BraveRewardsGetPendingContributionsTotalFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetPendingTotal(double amount);
};

class BraveRewardsGetRewardsMainEnabledFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRewardsMainEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetRewardsMainEnabledFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetRewardsMainEnabled(bool enabled);
};

class BraveRewardsSaveAdsSettingFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveAdsSetting", UNKNOWN)

 protected:
  ~BraveRewardsSaveAdsSettingFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetACEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getACEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetACEnabledFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetACEnabled(bool enabled);
};

class BraveRewardsSaveSettingFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveSetting", UNKNOWN)

 protected:
  ~BraveRewardsSaveSettingFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsSaveRecurringTipFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveRecurringTip", UNKNOWN)

 protected:
  ~BraveRewardsSaveRecurringTipFunction() override;

  ResponseAction Run() override;

 private:
  void OnSaveRecurringTip(bool success);
};

class BraveRewardsRemoveRecurringTipFunction :
  public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.removeRecurringTip", UNKNOWN)

 protected:
  ~BraveRewardsRemoveRecurringTipFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetRecurringTipsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRecurringTips", UNKNOWN)

 protected:
  ~BraveRewardsGetRecurringTipsFunction() override;

  ResponseAction Run() override;

 private:
    void OnGetRecurringTips(
        std::unique_ptr<brave_rewards::ContentSiteList> list);
};

class BraveRewardsGetPublisherBannerFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "braveRewards.getPublisherBanner", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherBannerFunction() override;

  ResponseAction Run() override;

 private:
  void OnPublisherBanner(
      std::unique_ptr<::brave_rewards::PublisherBanner> banner);
};

class BraveRewardsRefreshPublisherFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.refreshPublisher", UNKNOWN)

 protected:
  ~BraveRewardsRefreshPublisherFunction() override;

  ResponseAction Run() override;

 private:
  void OnRefreshPublisher(uint32_t status, const std::string& publisher_key);
};

class BraveRewardsGetAllNotificationsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAllNotifications", UNKNOWN)

 protected:
  ~BraveRewardsGetAllNotificationsFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetInlineTipSettingFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getInlineTipSetting", UNKNOWN)

 protected:
  ~BraveRewardsGetInlineTipSettingFunction() override;

  ResponseAction Run() override;

 private:
  void OnInlineTipSetting(bool value);
};

class BraveRewardsFetchBalanceFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.fetchBalance", UNKNOWN)

 protected:
  ~BraveRewardsFetchBalanceFunction() override;

  ResponseAction Run() override;

 private:
  void OnBalance(int32_t result,
                 std::unique_ptr<::brave_rewards::Balance> balance);
};

class BraveRewardsGetExternalWalletFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getExternalWallet", UNKNOWN)

 protected:
  ~BraveRewardsGetExternalWalletFunction() override;

  ResponseAction Run() override;

 private:
  void OnExternalWalet(
      int32_t result,
      std::unique_ptr<::brave_rewards::ExternalWallet> wallet);
};

class BraveRewardsDisconnectWalletFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.disconnectWallet", UNKNOWN)

 protected:
  ~BraveRewardsDisconnectWalletFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsOnlyAnonWalletFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.onlyAnonWallet", UNKNOWN)

 protected:
  ~BraveRewardsOnlyAnonWalletFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetAdsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsEnabledFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetAdsEstimatedEarningsFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsEstimatedEarnings", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsEstimatedEarningsFunction() override;

  ResponseAction Run() override;

 private:
  void OnAdsEstimatedEarnings(
      const double estimated_pending_rewards,
      const uint64_t next_payment_date_in_seconds,
      const uint64_t ad_notifications_received_this_month);
};

class BraveRewardsGetWalletExistsFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getWalletExists", UNKNOWN)

 protected:
  ~BraveRewardsGetWalletExistsFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetWalletExists(const bool exists);
};

class BraveRewardsGetAdsSupportedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsSupported", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsSupportedFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetAnonWalletStatusFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAnonWalletStatus", UNKNOWN)

 protected:
  ~BraveRewardsGetAnonWalletStatusFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetAnonWalletStatus(const uint32_t result);
};

// TODO(zeparsing): Development only
class BraveRewardsShowCheckoutDialogFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.showCheckoutDialog", UNKNOWN)

 protected:
  ~BraveRewardsShowCheckoutDialogFunction() override;

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
