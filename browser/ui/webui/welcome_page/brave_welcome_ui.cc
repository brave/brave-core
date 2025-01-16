/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/welcome_page/brave_welcome_ui.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/settings/brave_import_bulk_data_handler.h"
#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"
#include "brave/browser/ui/webui/welcome_page/welcome_dom_handler.h"
#include "brave/components/brave_welcome/common/features.h"
#include "brave/components/brave_welcome/resources/grit/brave_welcome_generated_map.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/settings/privacy_sandbox_handler.h"
#include "chrome/browser/ui/webui/settings/settings_default_browser_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/branded_strings.h"
#include "components/country_codes/country_codes.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {

constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"headerText", IDS_WELCOME_HEADER},
    {"braveWelcomeTitle", IDS_BRAVE_WELCOME_TITLE},
    {"braveWelcomeDesc", IDS_BRAVE_WELCOME_DESC},
    {"braveWelcomeImportSettingsTitle",
     IDS_BRAVE_WELCOME_IMPORT_SETTINGS_TITLE},
    {"braveWelcomeImportSettingsDesc", IDS_BRAVE_WELCOME_IMPORT_SETTINGS_DESC},
    {"braveWelcomeSelectProfileLabel", IDS_BRAVE_WELCOME_SELECT_PROFILE_LABEL},
    {"braveWelcomeSelectProfileDesc", IDS_BRAVE_WELCOME_SELECT_PROFILE_DESC},
    {"braveWelcomeImportButtonLabel", IDS_BRAVE_WELCOME_IMPORT_BUTTON_LABEL},
    {"braveWelcomeImportProfilesButtonLabel",
     IDS_BRAVE_WELCOME_IMPORT_PROFILES_BUTTON_LABEL},
    {"braveWelcomeSkipButtonLabel", IDS_BRAVE_WELCOME_SKIP_BUTTON_LABEL},
    {"braveWelcomeBackButtonLabel", IDS_BRAVE_WELCOME_BACK_BUTTON_LABEL},
    {"braveWelcomeNextButtonLabel", IDS_BRAVE_WELCOME_NEXT_BUTTON_LABEL},
    {"braveWelcomeFinishButtonLabel", IDS_BRAVE_WELCOME_FINISH_BUTTON_LABEL},
    {"braveWelcomeSetDefaultButtonLabel",
     IDS_BRAVE_WELCOME_SET_DEFAULT_BUTTON_LABEL},
    {"braveWelcomeSelectAllButtonLabel",
     IDS_BRAVE_WELCOME_SELECT_ALL_BUTTON_LABEL},
    {"braveWelcomeHelpImproveBraveTitle",
     IDS_BRAVE_WELCOME_HELP_IMPROVE_BRAVE_TITLE},
    {"braveWelcomeSendReportsLabel", IDS_BRAVE_WELCOME_SEND_REPORTS_LABEL},
    {"braveWelcomeSendInsightsLabel", IDS_BRAVE_WELCOME_SEND_INSIGHTS_LABEL},
    {"braveWelcomeSetupCompleteLabel", IDS_BRAVE_WELCOME_SETUP_COMPLETE_LABEL},
    {"braveWelcomeChangeSettingsNote", IDS_BRAVE_WELCOME_CHANGE_SETTINGS_NOTE},
    {"braveWelcomePrivacyPolicyNote", IDS_BRAVE_WELCOME_PRIVACY_POLICY_NOTE},
    {"braveWelcomeSelectThemeLabel", IDS_BRAVE_WELCOME_SELECT_THEME_LABEL},
    {"braveWelcomeSelectThemeNote", IDS_BRAVE_WELCOME_SELECT_THEME_NOTE},
    {"braveWelcomeSelectThemeSystemLabel",
     IDS_BRAVE_WELCOME_SELECT_THEME_SYSTEM_LABEL},
    {"braveWelcomeSelectThemeLightLabel",
     IDS_BRAVE_WELCOME_SELECT_THEME_LIGHT_LABEL},
    {"braveWelcomeSelectThemeDarkLabel",
     IDS_BRAVE_WELCOME_SELECT_THEME_DARK_LABEL},
    {"braveWelcomeHelpWDPTitle", IDS_BRAVE_WELCOME_HELP_WDP_TITLE},
    {"braveWelcomeHelpWDPSubtitle", IDS_BRAVE_WELCOME_HELP_WDP_SUBTITLE},
    {"braveWelcomeHelpWDPDescription", IDS_BRAVE_WELCOME_HELP_WDP_DESCRIPTION},
    {"braveWelcomeHelpWDPLearnMore", IDS_BRAVE_WELCOME_HELP_WDP_LEARN_MORE},
    {"braveWelcomeHelpWDPAccept", IDS_BRAVE_WELCOME_HELP_WDP_ACCEPT},
    {"braveWelcomeHelpWDPReject", IDS_BRAVE_WELCOME_HELP_WDP_REJECT}};

void OpenJapanWelcomePage(Profile* profile) {
  DCHECK(profile);
  Browser* browser = chrome::FindBrowserWithProfile(profile);
  if (browser) {
    content::OpenURLParams open_params(
        GURL("https://brave.com/ja/desktop-ntp-tutorial"), content::Referrer(),
        WindowOpenDisposition::NEW_BACKGROUND_TAB,
        ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
    browser->OpenURL(open_params, /*navigation_handle_callback=*/{});
  }
}

// Converts Chromium country ID to 2 digit country string
// For more info see src/components/country_codes/country_codes.h
std::string CountryIDToCountryString(int country_id) {
  if (country_id == country_codes::kCountryIDUnknown) {
    return std::string();
  }

  char chars[3] = {static_cast<char>(country_id >> 8),
                   static_cast<char>(country_id), 0};
  std::string country_string(chars);
  DCHECK_EQ(country_string.size(), 2U);
  return country_string;
}

}  // namespace

BraveWelcomeUI::BraveWelcomeUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui) {
  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, name, kBraveWelcomeGenerated, IDR_BRAVE_WELCOME_HTML,
      /*disable_trusted_types_csp=*/true);

  // Lottie animations tick on a worker thread and requires the document CSP to
  // be set to "worker-src blob: 'self';".
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc,
      "worker-src blob: chrome://resources 'self';");

  web_ui->AddMessageHandler(
      std::make_unique<WelcomeDOMHandler>(Profile::FromWebUI(web_ui)));
  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveImportBulkDataHandler>());
  web_ui->AddMessageHandler(
      std::make_unique<settings::DefaultBrowserHandler>());  // set default
                                                             // browser

  Profile* profile = Profile::FromWebUI(web_ui);
  // added to allow front end to read/modify default search engine
  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveSearchEnginesHandler>(profile));

  // Open additional page in Japanese region
  int country_id = country_codes::GetCountryIDFromPrefs(profile->GetPrefs());
  const bool is_jpn =
      country_id == country_codes::CountryStringToCountryID("JP");
  if (!profile->GetPrefs()->GetBoolean(prefs::kHasSeenWelcomePage)) {
    if (is_jpn) {
      base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE, base::BindOnce(&OpenJapanWelcomePage, profile),
          base::Seconds(3));
    }
  }

  for (const auto& str : kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    source->AddString(str.name, l10n_str);
  }

  // Variables considered when determining which onboarding cards to show
  source->AddString("countryString", CountryIDToCountryString(country_id));
  source->AddBoolean(
      "showRewardsCard",
      base::FeatureList::IsEnabled(brave_welcome::features::kShowRewardsCard));

  source->AddBoolean(
      "hardwareAccelerationEnabledAtStartup",
      content::GpuDataManager::GetInstance()->HardwareAccelerationEnabled());

  profile->GetPrefs()->SetBoolean(prefs::kHasSeenWelcomePage, true);

  AddBackgroundColorToSource(source, web_ui->GetWebContents());
}

BraveWelcomeUI::~BraveWelcomeUI() = default;
