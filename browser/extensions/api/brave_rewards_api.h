/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_

#include <memory>

#include "extensions/browser/extension_function.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"

namespace extensions {
namespace api {

class BraveRewardsCreateWalletFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.createWallet", UNKNOWN)

 protected:
  ~BraveRewardsCreateWalletFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsDonateToSiteFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.donateToSite", UNKNOWN)

 protected:
  ~BraveRewardsDonateToSiteFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetPublisherDataFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherData", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherDataFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetWalletPropertiesFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getWalletProperties", UNKNOWN)

 protected:
  ~BraveRewardsGetWalletPropertiesFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetCurrentReportFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getCurrentReport", UNKNOWN)

 protected:
  ~BraveRewardsGetCurrentReportFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsIncludeInAutoContributionFunction :
  public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.includeInAutoContribution", UNKNOWN)

 protected:
  ~BraveRewardsIncludeInAutoContributionFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetGrantsFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getGrants", UNKNOWN)

 protected:
  ~BraveRewardsGetGrantsFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetGrantCaptchaFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getGrantCaptcha", UNKNOWN)

 protected:
  ~BraveRewardsGetGrantCaptchaFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsSolveGrantCaptchaFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.solveGrantCaptcha", UNKNOWN)

 protected:
  ~BraveRewardsSolveGrantCaptchaFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetPendingContributionsTotalFunction
    : public UIThreadExtensionFunction {
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
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRewardsMainEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetRewardsMainEnabledFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetRewardsMainEnabled(bool enabled);
};

class BraveRewardsSaveAdsSettingFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveAdsSetting", UNKNOWN)

 protected:
  ~BraveRewardsSaveAdsSettingFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetACEnabledFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getACEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetACEnabledFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetACEnabled(bool enabled);
};

class BraveRewardsSaveSettingFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveSetting", UNKNOWN)

 protected:
  ~BraveRewardsSaveSettingFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsSaveRecurringDonationFunction :
  public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveRecurringDonation", UNKNOWN)

 protected:
  ~BraveRewardsSaveRecurringDonationFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsRemoveRecurringDonationFunction :
  public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.removeRecurringDonation", UNKNOWN)

 protected:
  ~BraveRewardsRemoveRecurringDonationFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetRecurringDonationsFunction :
  public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRecurringDonations", UNKNOWN)

 protected:
  ~BraveRewardsGetRecurringDonationsFunction() override;

  ResponseAction Run() override;

  private:
    void OnGetRecurringDonations(
        std::unique_ptr<brave_rewards::ContentSiteList> list);
};

class BraveRewardsGetPublisherBannerFunction :
public UIThreadExtensionFunction {
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

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
