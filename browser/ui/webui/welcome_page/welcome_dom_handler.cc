/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/welcome_page/welcome_dom_handler.h"

#include <algorithm>

#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/components/p3a/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(BRAVE_P3A_ENABLED)
#include "brave/components/p3a/pref_names.h"
#endif

namespace {

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

}  // namespace

WelcomeDOMHandler::WelcomeDOMHandler(Profile* profile) : profile_(profile) {
  base::MakeRefCounted<shell_integration::DefaultSchemeClientWorker>(
      GURL("https://brave.com"))
      ->StartCheckIsDefaultAndGetDefaultClientName(
          base::BindOnce(&WelcomeDOMHandler::OnGetDefaultBrowser,
                         weak_ptr_factory_.GetWeakPtr()));
}

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
  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(default_browser_name_));
}

void WelcomeDOMHandler::OnGetDefaultBrowser(
    shell_integration::DefaultWebClientState state,
    const std::u16string& name) {
  std::u16string browser_name = name;
  if (browser_name ==
      l10n_util::GetStringUTF16(IDS_CHROME_SHORTCUT_NAME_BETA)) {
    browser_name = base::UTF8ToUTF16(std::string(kGoogleChromeBrowserBeta));
  } else if (browser_name ==
             l10n_util::GetStringUTF16(IDS_CHROME_SHORTCUT_NAME_DEV)) {
    browser_name = base::UTF8ToUTF16(std::string(kGoogleChromeBrowserDev));
  }
  default_browser_name_ = browser_name;
}

void WelcomeDOMHandler::HandleRecordP3A(const base::Value::List& args) {
  if (!args[0].is_int() || !args[1].is_bool() || !args[2].is_bool()) {
    return;
  }
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

#if BUILDFLAG(BRAVE_P3A_ENABLED)
void WelcomeDOMHandler::SetP3AEnabled(const base::Value::List& args) {
  SetLocalStateBooleanEnabled(brave::kP3AEnabled, args);
}
#endif
