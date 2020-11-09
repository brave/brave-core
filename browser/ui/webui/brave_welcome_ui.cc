/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_welcome_ui.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/metrics/histogram_macros.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_welcome/resources/grit/brave_welcome_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/webui/settings/search_engines_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_message_handler.h"

using content::WebUIMessageHandler;

namespace {

void RecordP3AHistogram(int screen_number, bool finished) {
  int answer = 0;
  if (finished) {
    answer = 3;
  } else {
    answer = std::min(screen_number, 2);
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Welcome.InteractionStatus", answer, 3);
}

// The handler for Javascript messages for the chrome://welcome page
class WelcomeDOMHandler : public WebUIMessageHandler {
 public:
  WelcomeDOMHandler() {
  }
  ~WelcomeDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleImportNowRequested(const base::ListValue* args);
  void HandleRecordP3A(const base::ListValue* args);
  void OnWalletInitialized(int result_code);
  Browser* GetBrowser();

  int screen_number_ = 0;
  bool finished_ = false;
  bool skipped_ = false;

  DISALLOW_COPY_AND_ASSIGN(WelcomeDOMHandler);
};

WelcomeDOMHandler::~WelcomeDOMHandler() {
  RecordP3AHistogram(screen_number_, finished_);
}

Browser* WelcomeDOMHandler::GetBrowser() {
  return chrome::FindBrowserWithWebContents(web_ui()->GetWebContents());
}

void WelcomeDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("importNowRequested",
      base::BindRepeating(&WelcomeDOMHandler::HandleImportNowRequested,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback("recordP3A",
      base::BindRepeating(&WelcomeDOMHandler::HandleRecordP3A,
                          base::Unretained(this)));
}

void WelcomeDOMHandler::HandleImportNowRequested(const base::ListValue* args) {
  chrome::ShowSettingsSubPageInTabbedBrowser(GetBrowser(),
      chrome::kImportDataSubPage);
}

void WelcomeDOMHandler::HandleRecordP3A(const base::ListValue* args) {
  if (!args->GetInteger(0, &screen_number_))
    return;
  if (!args->GetBoolean(1, &finished_))
    return;
  if (!args->GetBoolean(2, &skipped_))
    return;

  if (screen_number_) {
    // It is 1-based on JS side, we want 0-based.
    screen_number_--;
  }
  RecordP3AHistogram(screen_number_, finished_);
}


}  // namespace

BraveWelcomeUI::BraveWelcomeUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kBraveWelcomeGenerated,
        kBraveWelcomeGeneratedSize, IDR_BRAVE_WELCOME_HTML,
        /*disable_trusted_types_csp=*/true) {
  web_ui->AddMessageHandler(std::make_unique<WelcomeDOMHandler>());
  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveImportDataHandler>());

  Profile* profile = Profile::FromWebUI(web_ui);

  // added to allow front end to read/modify default search engine
  web_ui->AddMessageHandler(
      std::make_unique<settings::SearchEnginesHandler>(profile));

  profile->GetPrefs()->SetBoolean(prefs::kHasSeenWelcomePage, true);
}

BraveWelcomeUI::~BraveWelcomeUI() {
}
