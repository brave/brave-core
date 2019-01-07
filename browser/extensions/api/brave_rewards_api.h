/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_

#include "extensions/browser/extension_function.h"

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

class BraveRewardsGetWalletPropertiesFunction : public UIThreadExtensionFunction {
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

class BraveRewardsGetGrantFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getGrant", UNKNOWN)

 protected:
  ~BraveRewardsGetGrantFunction() override;

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

class BraveRewardsGetNonVerifiedSettingsFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getNonVerifiedSettings", UNKNOWN)

 protected:
  ~BraveRewardsGetNonVerifiedSettingsFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetAllowNonVerified(bool non_verified);
};

class BraveRewardsGetPendingContributionsTotalFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPendingContributionsTotal", UNKNOWN)

 protected:
  ~BraveRewardsGetPendingContributionsTotalFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetPendingTotal(double amount);
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
