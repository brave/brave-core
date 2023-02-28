/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_welcome_ui.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/settings/brave_import_bulk_data_handler.h"
#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"
#include "brave/components/brave_welcome/common/features.h"
#include "brave/components/brave_welcome/resources/grit/brave_welcome_generated_map.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/p3a/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/webui/settings/privacy_sandbox_handler.h"
#include "chrome/browser/ui/webui/settings/settings_default_browser_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "common/importer/importer_constants.h"
#include "components/country_codes/country_codes.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

#if BUILDFLAG(BRAVE_P3A_ENABLED)
#include "brave/components/p3a/pref_names.h"
#endif

using content::WebUIMessageHandler;

namespace {

constexpr webui::LocalizedString kLocalizedStrings[] = {
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
     IDS_BRAVE_WELCOME_SELECT_THEME_DARK_LABEL}};

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

void RecordP3AHistogram(int screen_number, bool finished) {
  int kCurrentScreen = 0;
  int kMaxScreens = 6;
  if (finished) {
    kCurrentScreen = kMaxScreens + 1;
  } else {
    kCurrentScreen = std::min(screen_number, kMaxScreens);
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Welcome.InteractionStatus", kCurrentScreen,
                             kMaxScreens + 1);
}

// The handler for Javascript messages for the chrome://welcome page
class WelcomeDOMHandler : public WebUIMessageHandler {
 public:
  explicit WelcomeDOMHandler(Profile* profile);
  WelcomeDOMHandler(const WelcomeDOMHandler&) = delete;
  WelcomeDOMHandler& operator=(const WelcomeDOMHandler&) = delete;
  ~WelcomeDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleImportNowRequested(const base::Value::List& args);
  void HandleRecordP3A(const base::Value::List& args);
  void HandleGetDefaultBrowser(const base::Value::List& args);
  void SetLocalStateBooleanEnabled(const std::string& path,
                                   const base::Value::List& args);
  void OnGetDefaultBrowser(const std::string& callback_id,
                           shell_integration::DefaultWebClientState state,
                           const std::u16string& name);
  void SetP3AEnabled(const base::Value::List& args);
  void HandleOpenSettingsPage(const base::Value::List& args);
  void HandleSetMetricsReportingEnabled(const base::Value::List& args);
  Browser* GetBrowser();

  int screen_number_ = 0;
  bool finished_ = false;
  bool skipped_ = false;
  raw_ptr<Profile> profile_ = nullptr;
  base::WeakPtrFactory<WelcomeDOMHandler> weak_ptr_factory_{this};
};

WelcomeDOMHandler::WelcomeDOMHandler(Profile* profile) : profile_(profile) {}

WelcomeDOMHandler::~WelcomeDOMHandler() {
  RecordP3AHistogram(screen_number_, finished_);
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
  web_ui()->RegisterMessageCallback(
      "setP3AEnabled", base::BindRepeating(&WelcomeDOMHandler::SetP3AEnabled,
                                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "openSettingsPage",
      base::BindRepeating(&WelcomeDOMHandler::HandleOpenSettingsPage,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setMetricsReportingEnabled",
      base::BindRepeating(&WelcomeDOMHandler::HandleSetMetricsReportingEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDefaultBrowser",
      base::BindRepeating(&WelcomeDOMHandler::HandleGetDefaultBrowser,
                          base::Unretained(this)));
}

void WelcomeDOMHandler::HandleImportNowRequested(
    const base::Value::List& args) {
  chrome::ShowSettingsSubPageInTabbedBrowser(GetBrowser(),
                                             chrome::kImportDataSubPage);
}

void WelcomeDOMHandler::HandleGetDefaultBrowser(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  const auto& callback_id = args[0].GetString();
  AllowJavascript();

  base::MakeRefCounted<shell_integration::DefaultSchemeClientWorker>(
      GURL("https://brave.com"))
      ->StartCheckIsDefaultAndGetDefaultClientName(
          base::BindOnce(&WelcomeDOMHandler::OnGetDefaultBrowser,
                         weak_ptr_factory_.GetWeakPtr(), callback_id));
}

void WelcomeDOMHandler::OnGetDefaultBrowser(
    const std::string& callback_id,
    shell_integration::DefaultWebClientState state,
    const std::u16string& name) {
  std::u16string browser_name = name;
  if (browser_name == l10n_util::GetStringUTF16(IDS_BRAVE_SHORTCUT_NAME_BETA)) {
    browser_name = "Google Chrome Beta";
  } else if (browser_name ==
             l10n_util::GetStringUTF16(IDS_BRAVE_SHORTCUT_NAME_DEV)) {
    browser_name = "Google Chrome Dev";
  }
  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(browser_name));
}

void WelcomeDOMHandler::HandleRecordP3A(const base::Value::List& args) {
  if (!args[0].is_int() || !args[1].is_bool() || !args[2].is_bool())
    return;
  screen_number_ = args[0].GetInt();
  finished_ = args[1].GetBool();
  skipped_ = args[2].GetBool();

  RecordP3AHistogram(screen_number_, finished_);
}

void WelcomeDOMHandler::HandleOpenSettingsPage(const base::Value::List& args) {
  DCHECK(profile_);
  Browser* browser = chrome::FindBrowserWithProfile(profile_);
  if (browser) {
    content::OpenURLParams open_params(
        GURL("brave://settings/privacy"), content::Referrer(),
        WindowOpenDisposition::NEW_BACKGROUND_TAB,
        ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
    browser->OpenURL(open_params);
  }
}

void WelcomeDOMHandler::HandleSetMetricsReportingEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  if (!args[0].is_bool())
    return;
  bool enabled = args[0].GetBool();
  ChangeMetricsReportingState(
      enabled, ChangeMetricsReportingStateCalledFrom::kUiSettings);
}

void WelcomeDOMHandler::SetLocalStateBooleanEnabled(
    const std::string& path,
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  if (!args[0].is_bool())
    return;

  bool enabled = args[0].GetBool();
  PrefService* local_state = g_browser_process->local_state();
  local_state->SetBoolean(path, enabled);
}

#if BUILDFLAG(BRAVE_P3A_ENABLED)
void WelcomeDOMHandler::SetP3AEnabled(const base::Value::List& args) {
  SetLocalStateBooleanEnabled(brave::kP3AEnabled, args);
}
#endif

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
  if (!profile->GetPrefs()->GetBoolean(prefs::kHasSeenWelcomePage)) {
    if (country_id == country_codes::CountryStringToCountryID("JP")) {
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
