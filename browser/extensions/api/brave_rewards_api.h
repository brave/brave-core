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

#include "base/types/expected.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/mojom_structs.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom.h"
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

class BraveRewardsIsUnsupportedRegionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.isUnsupportedRegion", UNKNOWN)

 protected:
  ~BraveRewardsIsUnsupportedRegionFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetLocaleFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getLocale", UNKNOWN)

 protected:
  ~BraveRewardsGetLocaleFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsOpenRewardsPanelFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.openRewardsPanel", UNKNOWN)

 protected:
  ~BraveRewardsOpenRewardsPanelFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsShowGrantCaptchaFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.showGrantCaptcha", UNKNOWN)

 protected:
  ~BraveRewardsShowGrantCaptchaFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsUpdateMediaDurationFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.updateMediaDuration", UNKNOWN)

 protected:
  ~BraveRewardsUpdateMediaDurationFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetPublisherInfoFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherInfo", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherInfoFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetPublisherInfo(const ledger::mojom::Result result,
                          ledger::mojom::PublisherInfoPtr info);
};

class BraveRewardsSetPublisherIdForTabFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.setPublisherIdForTab", UNKNOWN)

 protected:
  ~BraveRewardsSetPublisherIdForTabFunction() override;
  ResponseAction Run() override;
};

class BraveRewardsGetPublisherInfoForTabFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherInfoForTab", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherInfoForTabFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetPublisherPanelInfo(ledger::mojom::Result result,
                               ledger::mojom::PublisherInfoPtr info);
};

class BraveRewardsGetPublisherPanelInfoFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherPanelInfo", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherPanelInfoFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetPublisherPanelInfo(const ledger::mojom::Result result,
                               ledger::mojom::PublisherInfoPtr info);
};

class BraveRewardsSavePublisherInfoFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.savePublisherInfo", UNKNOWN)

 protected:
  ~BraveRewardsSavePublisherInfoFunction() override;

  ResponseAction Run() override;

 private:
  void OnSavePublisherInfo(const ledger::mojom::Result result);
};

class BraveRewardsTipSiteFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.tipSite", UNKNOWN)

 protected:
  ~BraveRewardsTipSiteFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsTipUserFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.tipUser", UNKNOWN)

 protected:
  ~BraveRewardsTipUserFunction() override;

  ResponseAction Run() override;

 private:
  void OnTipUserGetPublisherInfo(const ledger::mojom::Result result,
                                 ledger::mojom::PublisherInfoPtr info);
  void OnTipUserSavePublisherInfo(const ledger::mojom::Result result);
  void ShowTipDialog();
};

class BraveRewardsGetPublisherDataFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPublisherData", UNKNOWN)

 protected:
  ~BraveRewardsGetPublisherDataFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetRewardsParametersFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRewardsParameters", UNKNOWN)

 protected:
  ~BraveRewardsGetRewardsParametersFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetRewardsParameters(ledger::mojom::RewardsParametersPtr parameters);
};

class BraveRewardsCreateRewardsWalletFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.createRewardsWallet", UNKNOWN)

 protected:
  ~BraveRewardsCreateRewardsWalletFunction() override;

  ResponseAction Run() override;

 private:
  void CreateRewardsWalletCallback(
      ledger::mojom::CreateRewardsWalletResult result);
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

class BraveRewardsGetDeclaredCountryFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getDeclaredCountry", UNKNOWN)
 protected:
  ~BraveRewardsGetDeclaredCountryFunction() override;
  ResponseAction Run() override;
};

class BraveRewardsGetUserVersionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getUserVersion", UNKNOWN)
 protected:
  ~BraveRewardsGetUserVersionFunction() override;
  ResponseAction Run() override;
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
  void OnBalanceReport(const ledger::mojom::Result result,
                       ledger::mojom::BalanceReportInfoPtr report);
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

 private:
  void OnPromotionsFetched(std::vector<ledger::mojom::PromotionPtr> promotions);
};

class BraveRewardsClaimPromotionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.claimPromotion", UNKNOWN)

 protected:
  ~BraveRewardsClaimPromotionFunction() override;

  ResponseAction Run() override;

 private:
  void OnClaimPromotion(const std::string& promotion_id,
                        const ledger::mojom::Result result,
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
  void OnAttestPromotion(const std::string& promotion_id,
                         const ledger::mojom::Result result,
                         ledger::mojom::PromotionPtr promotion);
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

class BraveRewardsSaveAdsSettingFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveAdsSetting", UNKNOWN)

 protected:
  ~BraveRewardsSaveAdsSettingFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsSetAutoContributeEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.setAutoContributeEnabled", UNKNOWN)

 protected:
  ~BraveRewardsSetAutoContributeEnabledFunction() override;

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

class BraveRewardsSaveRecurringTipFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.saveRecurringTip", UNKNOWN)

 protected:
  ~BraveRewardsSaveRecurringTipFunction() override;

  ResponseAction Run() override;

 private:
  void OnSaveRecurringTip(ledger::mojom::Result result);
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
  void OnGetRecurringTips(std::vector<ledger::mojom::PublisherInfoPtr> list);
};

class BraveRewardsRefreshPublisherFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.refreshPublisher", UNKNOWN)

 protected:
  ~BraveRewardsRefreshPublisherFunction() override;

  ResponseAction Run() override;

 private:
  void OnRefreshPublisher(const ledger::mojom::PublisherStatus status,
                          const std::string& publisher_key);
};

class BraveRewardsGetAllNotificationsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAllNotifications", UNKNOWN)

 protected:
  ~BraveRewardsGetAllNotificationsFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetInlineTippingPlatformEnabledFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "braveRewards.getInlineTippingPlatformEnabled",
      UNKNOWN)

 protected:
  ~BraveRewardsGetInlineTippingPlatformEnabledFunction() override;

  ResponseAction Run() override;

 private:
  void OnInlineTipSetting(bool value);
};

class BraveRewardsIsAutoContributeSupportedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.isAutoContributeSupported", UNKNOWN)

 protected:
  ~BraveRewardsIsAutoContributeSupportedFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsFetchBalanceFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.fetchBalance", UNKNOWN)

 protected:
  ~BraveRewardsFetchBalanceFunction() override;

  ResponseAction Run() override;

 private:
  void OnBalance(const ledger::mojom::Result result,
                 ledger::mojom::BalancePtr balance);
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
  void OnGetExternalWallet(
      base::expected<ledger::mojom::ExternalWalletPtr,
                     ledger::mojom::GetExternalWalletError> result);
};

class BraveRewardsGetRewardsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getRewardsEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetRewardsEnabledFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetAdsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsEnabled", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsEnabledFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetAdsAccountStatementFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsAccountStatement", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsAccountStatementFunction() override;

  ResponseAction Run() override;

 private:
  void OnGetAdsAccountStatement(ads::mojom::StatementInfoPtr statement);
};

class BraveRewardsGetAdsSupportedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsSupported", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsSupportedFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetAdsDataFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getAdsData", UNKNOWN)

 protected:
  ~BraveRewardsGetAdsDataFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsIsInitializedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.isInitialized", UNKNOWN)

 protected:
  ~BraveRewardsIsInitializedFunction() override;

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

class BraveRewardsEnableAdsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.enableAds", UNKNOWN)

 protected:
  ~BraveRewardsEnableAdsFunction() override;

  ResponseAction Run() override;
};

class BraveRewardsGetPrefsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.getPrefs", UNKNOWN)

 protected:
  ~BraveRewardsGetPrefsFunction() override;

  ResponseAction Run() override;

 private:
  void GetAutoContributePropertiesCallback(
      ledger::mojom::AutoContributePropertiesPtr properties);
};

class BraveRewardsUpdatePrefsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveRewards.updatePrefs", UNKNOWN)

 protected:
  ~BraveRewardsUpdatePrefsFunction() override;

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_REWARDS_API_H_
