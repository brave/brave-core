/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/browser/balance.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/external_wallet.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
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

class BraveRewardsGetCurrentReportFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getCurrentReport", UNKNOWN)

 protected:
  ~BraveRewardsGetCurrentReportFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsIncludeInAutoContributionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.includeInAutoContribution", UNKNOWN)

 protected:
  ~BraveRewardsIncludeInAutoContributionFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetGrantsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getGrants", UNKNOWN)

 protected:
  ~BraveRewardsGetGrantsFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetGrantCaptchaFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getGrantCaptcha", UNKNOWN)

 protected:
  ~BraveRewardsGetGrantCaptchaFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsSolveGrantCaptchaFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.solveGrantCaptcha", UNKNOWN)

 protected:
  ~BraveRewardsSolveGrantCaptchaFunction() override;

  ResponseAction Run() override;
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

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
