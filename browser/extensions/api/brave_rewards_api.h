/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveRewardsIsSupportedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.isSupported", UNKNOWN)

 protected:
  ~BraveRewardsIsSupportedFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsRecordNTPPanelTriggerFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.recordNTPPanelTrigger", UNKNOWN)

 protected:
  ~BraveRewardsRecordNTPPanelTriggerFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsOpenRewardsPanelFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.openRewardsPanel", UNKNOWN)

 protected:
  ~BraveRewardsOpenRewardsPanelFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetPublisherInfoFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherInfo", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherInfoFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetPublisherInfo(const brave_rewards::mojom::Result result,
                          brave_rewards::mojom::PublisherInfoPtr info);
};

class BraveRewardsGetPublisherInfoForTabFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherInfoForTab", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherInfoForTabFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetPublisherPanelInfo(brave_rewards::mojom::Result result,
                               brave_rewards::mojom::PublisherInfoPtr info);
};

class BraveRewardsTipSiteFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.tipSite", UNKNOWN)

 protected:
  ~BraveRewardsTipSiteFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetRewardsParametersFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRewardsParameters", UNKNOWN)

 protected:
  ~BraveRewardsGetRewardsParametersFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetRewardsParameters(
      brave_rewards::mojom::RewardsParametersPtr parameters);
};

class BraveRewardsCreateRewardsWalletFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.createRewardsWallet", UNKNOWN)

 protected:
  ~BraveRewardsCreateRewardsWalletFunction() override;

  ResponseAction Run() override;

 private:
  void CreateRewardsWalletCallback(
      brave_rewards::mojom::CreateRewardsWalletResult result);
};

class BraveRewardsGetAvailableCountriesFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAvailableCountries", UNKNOWN)

 protected:
  ~BraveRewardsGetAvailableCountriesFunction() override;

 private:
  void GetAvailableCountriesCallback(std::vector<std::string> countries);

  ResponseAction Run() override;
};

class BraveRewardsGetDefaultCountryFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getDefaultCountry", UNKNOWN)

 protected:
  ~BraveRewardsGetDefaultCountryFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetDeclaredCountryFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getDeclaredCountry", UNKNOWN)
 protected:
  ~BraveRewardsGetDeclaredCountryFunction() override;
  ResponseAction Run() override;
};

class BraveRewardsGetUserTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getUserType", UNKNOWN)

 protected:
  ~BraveRewardsGetUserTypeFunction() override;
  ResponseAction Run() override;

 private:
  void Callback(brave_rewards::mojom::UserType user_type);
};

class BraveRewardsGetPublishersVisitedCountFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublishersVisitedCount", UNKNOWN)

 protected:
  ~BraveRewardsGetPublishersVisitedCountFunction() override;

  ResponseAction Run() override;

 private:
  void Callback(int count);
};

class BraveRewardsGetBalanceReportFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getBalanceReport", UNKNOWN)

 protected:
  ~BraveRewardsGetBalanceReportFunction() override;

  ResponseAction Run() override;

 private:
  void OnBalanceReport(const brave_rewards::mojom::Result result,
                       brave_rewards::mojom::BalanceReportInfoPtr report);
};

class BraveRewardsSaveRecurringTipFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveRecurringTip", UNKNOWN)

 protected:
  ~BraveRewardsSaveRecurringTipFunction() override;

  ResponseAction Run() override;

 private:
  void OnSaveRecurringTip(brave_rewards::mojom::Result result);
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
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list);
};

class BraveRewardsRefreshPublisherFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.refreshPublisher", UNKNOWN)

 protected:
  ~BraveRewardsRefreshPublisherFunction() override;

  ResponseAction Run() override;

 private:
  void OnRefreshPublisher(const brave_rewards::mojom::PublisherStatus status,
                          const std::string& publisher_key);
};

class BraveRewardsGetAllNotificationsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAllNotifications", UNKNOWN)

 protected:
  ~BraveRewardsGetAllNotificationsFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsFetchBalanceFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.fetchBalance", UNKNOWN)

 protected:
  ~BraveRewardsFetchBalanceFunction() override;

  ResponseAction Run() override;

 private:
  void OnFetchBalance(brave_rewards::mojom::BalancePtr balance);
};

class BraveRewardsGetExternalWalletProvidersFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getExternalWalletProviders", UNKNOWN)

 protected:
  ~BraveRewardsGetExternalWalletProvidersFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetExternalWalletFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getExternalWallet", UNKNOWN)

 protected:
  ~BraveRewardsGetExternalWalletFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetExternalWallet(brave_rewards::mojom::ExternalWalletPtr wallet);
};

class BraveRewardsGetRewardsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRewardsEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetRewardsEnabledFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetAdsAccountStatementFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsAccountStatement", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsAccountStatementFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetAdsAccountStatement(brave_ads::mojom::StatementInfoPtr statement);
};

class BraveRewardsIsInitializedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.isInitialized", UNKNOWN)

 protected:
  ~BraveRewardsIsInitializedFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsSelfCustodyInviteDismissedFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.selfCustodyInviteDismissed", UNKNOWN)

 protected:
  ~BraveRewardsSelfCustodyInviteDismissedFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsDismissSelfCustodyInviteFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.dismissSelfCustodyInvite", UNKNOWN)

 protected:
  ~BraveRewardsDismissSelfCustodyInviteFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsIsTermsOfServiceUpdateRequiredFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.isTermsOfServiceUpdateRequired",
                             UNKNOWN)

 protected:
  ~BraveRewardsIsTermsOfServiceUpdateRequiredFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsAcceptTermsOfServiceUpdateFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.acceptTermsOfServiceUpdate", UNKNOWN)

 protected:
  ~BraveRewardsAcceptTermsOfServiceUpdateFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetScheduledCaptchaInfoFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getScheduledCaptchaInfo", UNKNOWN)

 protected:
  ~BraveRewardsGetScheduledCaptchaInfoFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsUpdateScheduledCaptchaResultFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.updateScheduledCaptchaResult",
                             UNKNOWN)

 protected:
  ~BraveRewardsUpdateScheduledCaptchaResultFunction() override;

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
