/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_page_data_source.h"

#include "base/feature_list.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/rewards_page_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace brave_rewards {

namespace {

static constexpr webui::ResourcePath kResources[] = {
    {"favicon.ico", IDR_BRAVE_REWARDS_FAVICON}};

static constexpr webui::LocalizedString kStrings[] = {
    {"appErrorTitle", IDS_REWARDS_APP_ERROR_TITLE},
    {"cancelButtonLabel", IDS_REWARDS_PANEL_CANCEL},
    {"closeButtonLabel", IDS_BRAVE_REWARDS_ONBOARDING_CLOSE},
    {"connectAccountSubtext", IDS_REWARDS_CONNECT_ACCOUNT_SUBTEXT},
    {"connectAccountText", IDS_REWARDS_CONNECT_ACCOUNT_TEXT_2},
    {"connectButtonLabel", IDS_REWARDS_CONNECT_ACCOUNT},
    {"continueButtonLabel", IDS_BRAVE_REWARDS_ONBOARDING_CONTINUE},
    {"countrySelectPlaceholder", IDS_BRAVE_REWARDS_ONBOARDING_SELECT_COUNTRY},
    {"countrySelectTitle", IDS_REWARDS_COUNTRY_SELECT_TITLE},
    {"countrySelectText", IDS_REWARDS_COUNTRY_SELECT_TEXT},
    {"doneButtonLabel", IDS_BRAVE_REWARDS_ONBOARDING_DONE},
    {"helpButtonLabel", IDS_REWARDS_HELP_BUTTON_LABEL},
    {"moreButtonLabel", IDS_REWARDS_MORE_BUTTON_LABEL},
    {"navigationCreatorsLabel", IDS_REWARDS_NAVIGATION_CREATORS_LABEL},
    {"navigationExploreLabel", IDS_REWARDS_NAVIGATION_EXPLORE_LABEL},
    {"navigationHomeLabel", IDS_REWARDS_NAVIGATION_HOME_LABEL},
    {"onboardingButtonLabel", IDS_REWARDS_ONBOARDING_BUTTON_LABEL},
    {"onboardingErrorCountryDeclaredText",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DECLARE_COUNTRY},
    {"onboardingErrorDisabledText",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DISABLED},
    {"onboardingErrorDisabledTitle",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER_DISABLED},
    {"onboardingErrorText", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT},
    {"onboardingErrorTitle", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER},
    {"onboardingLearnMoreLabel", IDS_REWARDS_LEARN_MORE},
    {"onboardingSuccessLearnMoreLabel",
     IDS_BRAVE_REWARDS_ONBOARDING_HOW_DOES_IT_WORK},
    {"onboardingSuccessText", IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_TEXT},
    {"onboardingSuccessTitle", IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_HEADER},
    {"onboardingTermsText", IDS_REWARDS_ONBOARDING_TERMS_TEXT},
    {"onboardingTextItem1", IDS_REWARDS_ONBOARDING_TEXT_ITEM_1},
    {"onboardingTextItem2", IDS_REWARDS_ONBOARDING_TEXT_ITEM_2},
    {"onboardingTitle", IDS_REWARDS_ONBOARDING_TITLE},
    {"resetButtonLabel", IDS_BRAVE_UI_RESET},
    {"resetConsentText", IDS_BRAVE_UI_REWARDS_RESET_CONSENT},
    {"resetRewardsText", IDS_BRAVE_UI_REWARDS_RESET_TEXT},
    {"resetRewardsTitle", IDS_BRAVE_UI_RESET_WALLET},
    {"rewardsPageTitle", IDS_REWARDS_PAGE_TITLE}};

}  // namespace

void CreateAndAddRewardsPageDataSource(content::WebUI& web_ui,
                                       const std::string& host) {
  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui.GetWebContents()->GetBrowserContext(), host);

  webui::SetupWebUIDataSource(
      source, base::make_span(kRewardsPageGenerated, kRewardsPageGeneratedSize),
      IDR_NEW_BRAVE_REWARDS_PAGE_HTML);

  // Override img-src to allow chrome://rewards-image support.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://resources chrome://theme chrome://rewards-image "
      "chrome://favicon2 blob: data: 'self';");

  source->AddResourcePaths(kResources);
  source->AddLocalizedStrings(kStrings);

#if BUILDFLAG(IS_ANDROID)
  source->AddString("platform", "android");
#else
  source->AddString("platform", "desktop");
#endif

  source->AddBoolean("isBubble", host == kRewardsPageTopHost);

  source->AddBoolean(
      "animatedBackgroundEnabled",
      base::FeatureList::IsEnabled(features::kAnimatedBackgroundFeature));
}

}  // namespace brave_rewards
