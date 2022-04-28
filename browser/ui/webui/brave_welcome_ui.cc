/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_welcome_ui.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"
#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"
#include "brave/components/brave_welcome/common/features.h"
#include "brave/components/brave_welcome/resources/grit/brave_welcome_generated_map.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/country_codes/country_codes.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

using content::WebUIMessageHandler;

namespace {

void OpenJapanWelcomePage(Profile* profile) {
  DCHECK(profile);
  Browser* browser = chrome::FindBrowserWithProfile(profile);
  if (browser) {
    content::OpenURLParams open_params(
        GURL("https://brave.com/ja/desktop-ntp-tutorial"), content::Referrer(),
        WindowOpenDisposition::NEW_BACKGROUND_TAB,
        ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
    browser->OpenURL(open_params);
  }
}

// Returns whether the P3A opt-in prompt should be shown.
bool IsP3AOptInEnabled() {
  bool enabled = false;
  // Check the local_state pref for study status.
  PrefService* local_state = g_browser_process->local_state();
  if (local_state->GetInteger(brave::kP3AOptIn) ==
      brave::P3AOptInStage::kOptIn) {
    enabled = true;
  }
  VLOG(1) << "IsP3AOptInEnabled: " << enabled;
  return enabled;
}

void RecordP3AHistogram(int screen_number, bool finished) {
  int answer = 0;  // Did not click within the welcome screen
  if (finished) {
    answer = 3;  // Made it to the end of the on-boarding flow
  } else {
    // clicked once, or more, but didn't finish
    answer = std::min(screen_number, 2);
  }
  VLOG(1) << "RecordP3AHistogram value " << answer;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Welcome.InteractionStatus", answer, 3);
}

void MaybeRecordP3AOptIn(int screen_number, bool finished, bool opt_in) {
  // Screen number (zero-indexed) where we show the prompt.
  // Currently the same as shieldsBox.
  constexpr int p3a_screen = 2;

  // Record nothing if the feature is disabled.
  if (!IsP3AOptInEnabled()) {
    return;
  }
  VLOG(2) << "MaybeRecordP3AOptIn screen_number " << screen_number << " opt_in "
          << opt_in;

  int answer = 0;  // Did not see prompt.
  if (screen_number >= p3a_screen) {
    answer = 1 + opt_in;  // Saw prompt, didn't or did opt in.
  }
  VLOG(2) << "MaybeRecordP3AOptIn histogram value " << answer;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Welcome.P3AOptIn", answer, 2);

  // Record the user's preference if they clicked 'Done'
  if (finished) {
    VLOG(1) << "Recording user P3A preference: "
            << (opt_in ? "enable" : "disable");
    PrefService* local_state = g_browser_process->local_state();
    local_state->SetBoolean(brave::kP3AEnabled, opt_in);
  }
}

// The handler for Javascript messages for the chrome://welcome page
class WelcomeDOMHandler : public WebUIMessageHandler {
 public:
  WelcomeDOMHandler() = default;
  WelcomeDOMHandler(const WelcomeDOMHandler&) = delete;
  WelcomeDOMHandler& operator=(const WelcomeDOMHandler&) = delete;
  ~WelcomeDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleImportNowRequested(const base::Value::List& args);
  void HandleRecordP3A(const base::Value::List& args);
  Browser* GetBrowser();

  int screen_number_ = 0;
  bool finished_ = false;
  bool skipped_ = false;
  bool p3a_opt_in_ = false;
};

WelcomeDOMHandler::~WelcomeDOMHandler() {
  VLOG(1) << "WelcomeDOMHandler dtor: recording p3a values";
  RecordP3AHistogram(screen_number_, finished_);
  MaybeRecordP3AOptIn(screen_number_, finished_, p3a_opt_in_);
}

Browser* WelcomeDOMHandler::GetBrowser() {
  return chrome::FindBrowserWithWebContents(web_ui()->GetWebContents());
}

void WelcomeDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "importNowRequested",
      base::BindRepeating(&WelcomeDOMHandler::HandleImportNowRequested,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "recordP3A", base::BindRepeating(&WelcomeDOMHandler::HandleRecordP3A,
                                       base::Unretained(this)));
}

void WelcomeDOMHandler::HandleImportNowRequested(
    const base::Value::List& args) {
  chrome::ShowSettingsSubPageInTabbedBrowser(GetBrowser(),
                                             chrome::kImportDataSubPage);
}

void WelcomeDOMHandler::HandleRecordP3A(const base::Value::List& args) {
  if (!args[0].is_int() || !args[1].is_bool() || !args[2].is_bool() ||
      !args[3].is_bool()) {
    return;
  }
  screen_number_ = args[0].GetInt();
  finished_ = args[1].GetBool();
  skipped_ = args[2].GetBool();
  p3a_opt_in_ = args[3].GetBool();

  VLOG(1) << "HandleRecordP3A "
          << "screen_number: " << screen_number_ << ", "
          << "finished: " << finished_ << ", "
          << "skipped: " << skipped_ << ", "
          << "p3a_opt_in: " << p3a_opt_in_;

  if (screen_number_) {
    // It is 1-based on JS side, we want 0-based.
    screen_number_--;
  }
  RecordP3AHistogram(screen_number_, finished_);
  MaybeRecordP3AOptIn(screen_number_, finished_, p3a_opt_in_);
}

// Converts Chromium country ID to 2 digit country string
// For more info see src/components/country_codes/country_codes.h
std::string CountryIDToCountryString(int country_id) {
  if (country_id == country_codes::kCountryIDUnknown)
    return std::string();

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
      web_ui, name, kBraveWelcomeGenerated, kBraveWelcomeGeneratedSize,
      IDR_BRAVE_WELCOME_HTML,
      /*disable_trusted_types_csp=*/true);

  web_ui->AddMessageHandler(std::make_unique<WelcomeDOMHandler>());
  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveImportDataHandler>());

  Profile* profile = Profile::FromWebUI(web_ui);
  // added to allow front end to read/modify default search engine
  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveSearchEnginesHandler>(profile));

  // Open additional page in Japanese region
  int country_id = country_codes::GetCountryIDFromPrefs(profile->GetPrefs());
  if (!profile->GetPrefs()->GetBoolean(prefs::kHasSeenWelcomePage)) {
    if (country_id == country_codes::CountryStringToCountryID("JP")) {
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE, base::BindOnce(&OpenJapanWelcomePage, profile),
          base::Seconds(3));
    }
  }
  // Variables considered when determining which onboarding cards to show
  source->AddString("countryString", CountryIDToCountryString(country_id));
  source->AddBoolean(
      "showRewardsCard",
      base::FeatureList::IsEnabled(brave_welcome::features::kShowRewardsCard));
  source->AddBoolean("showP3AOptIn", IsP3AOptInEnabled());

  profile->GetPrefs()->SetBoolean(prefs::kHasSeenWelcomePage, true);
}

BraveWelcomeUI::~BraveWelcomeUI() = default;
