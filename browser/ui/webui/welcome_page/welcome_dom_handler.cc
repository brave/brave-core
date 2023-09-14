/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/welcome_page/welcome_dom_handler.h"

#include <algorithm>

#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/branded_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

constexpr char16_t kChromeBetaMacBrowserName[] = u"Chrome Beta";
constexpr char16_t kChromeDevMacBrowserName[] = u"Chrome Dev";
constexpr char16_t kChromeBetaLinuxBrowserName[] = u"Google Chrome (beta)";
constexpr char16_t kChromeDevLinuxBrowserName[] = u"Google Chrome (unstable)";
constexpr char kP3AOnboardingHistogramName[] =
    "Brave.Welcome.InteractionStatus.2";
constexpr size_t kMaxP3AOnboardingPhases = 3;

// What was the last screen that you viewed during the browser onboarding
// process?
// 0. Only viewed the welcome screen, performed no action
// 1. Viewed the profile import screen
// 2. Viewed the diagnostic/analytics consent screen
// 3. Finished the onboarding process
void RecordP3AHistogram(size_t last_onboarding_phase) {
  int answer = std::min(last_onboarding_phase, kMaxP3AOnboardingPhases);
  UMA_HISTOGRAM_EXACT_LINEAR(kP3AOnboardingHistogramName, answer,
                             kMaxP3AOnboardingPhases + 1);
}

bool IsChromeBeta(const std::u16string& browser_name) {
  return browser_name ==
             l10n_util::GetStringUTF16(IDS_CHROME_SHORTCUT_NAME_BETA) ||
         browser_name == kChromeBetaMacBrowserName ||
         browser_name == kChromeBetaLinuxBrowserName;
}

bool IsChromeDev(const std::u16string& browser_name) {
  return browser_name ==
             l10n_util::GetStringUTF16(IDS_CHROME_SHORTCUT_NAME_DEV) ||
         browser_name == kChromeDevMacBrowserName ||
         browser_name == kChromeDevLinuxBrowserName;
}

}  // namespace

WelcomeDOMHandler::WelcomeDOMHandler(Profile* profile) : profile_(profile) {
  base::MakeRefCounted<shell_integration::DefaultSchemeClientWorker>(
      GURL("https://brave.com"))
      ->StartCheckIsDefaultAndGetDefaultClientName(
          base::BindOnce(&WelcomeDOMHandler::OnGetDefaultBrowser,
                         weak_ptr_factory_.GetWeakPtr()));
}

WelcomeDOMHandler::~WelcomeDOMHandler() {
  RecordP3AHistogram(last_onboarding_phase_);
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
  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(default_browser_name_));
}

void WelcomeDOMHandler::OnGetDefaultBrowser(
    shell_integration::DefaultWebClientState state,
    const std::u16string& name) {
  std::u16string browser_name = name;
#if BUILDFLAG(IS_MAC)
  base::ReplaceSubstringsAfterOffset(&browser_name, 0, u".app", u"");
#endif
  if (IsChromeBeta(browser_name)) {
    browser_name = base::UTF8ToUTF16(std::string(kGoogleChromeBrowserBeta));
  } else if (IsChromeDev(browser_name)) {
    browser_name = base::UTF8ToUTF16(std::string(kGoogleChromeBrowserDev));
  }
  default_browser_name_ = browser_name;
}

void WelcomeDOMHandler::HandleRecordP3A(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  CHECK(args[0].is_int());

  last_onboarding_phase_ = args[0].GetInt();

  RecordP3AHistogram(last_onboarding_phase_);
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
  if (!args[0].is_bool()) {
    return;
  }
  bool enabled = args[0].GetBool();
  ChangeMetricsReportingState(
      enabled, ChangeMetricsReportingStateCalledFrom::kUiSettings);
}

void WelcomeDOMHandler::SetLocalStateBooleanEnabled(
    const std::string& path,
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  if (!args[0].is_bool()) {
    return;
  }

  bool enabled = args[0].GetBool();
  PrefService* local_state = g_browser_process->local_state();
  local_state->SetBoolean(path, enabled);
}

void WelcomeDOMHandler::SetP3AEnabled(const base::Value::List& args) {
  SetLocalStateBooleanEnabled(p3a::kP3AEnabled, args);
}
