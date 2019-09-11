/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_welcome_ui.h"

#include <memory>
#include <string>

#include "brave/browser/brave_browser_process_impl.h"
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

// The handler for Javascript messages for the chrome://welcome page
class WelcomeDOMHandler : public WebUIMessageHandler {
 public:
  WelcomeDOMHandler() {
  }
  ~WelcomeDOMHandler() override {}

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleImportNowRequested(const base::ListValue* args);
  void OnWalletInitialized(int result_code);
  Browser* GetBrowser();
  DISALLOW_COPY_AND_ASSIGN(WelcomeDOMHandler);
};

Browser* WelcomeDOMHandler::GetBrowser() {
  return chrome::FindBrowserWithWebContents(web_ui()->GetWebContents());
}

void WelcomeDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("importNowRequested",
      base::BindRepeating(&WelcomeDOMHandler::HandleImportNowRequested,
                          base::Unretained(this)));
}

void WelcomeDOMHandler::HandleImportNowRequested(const base::ListValue* args) {
  chrome::ShowSettingsSubPageInTabbedBrowser(GetBrowser(),
      chrome::kImportDataSubPage);
}

}  // namespace

BraveWelcomeUI::BraveWelcomeUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kBraveWelcomeGenerated,
        kBraveWelcomeGeneratedSize, IDR_BRAVE_WELCOME_HTML) {
  web_ui->AddMessageHandler(std::make_unique<WelcomeDOMHandler>());

  Profile* profile = Profile::FromWebUI(web_ui);

  // added to allow front end to read/modify default search engine
  web_ui->AddMessageHandler(
      std::make_unique<settings::SearchEnginesHandler>(profile));

  profile->GetPrefs()->SetBoolean(prefs::kHasSeenWelcomePage, true);
}

BraveWelcomeUI::~BraveWelcomeUI() {
}
