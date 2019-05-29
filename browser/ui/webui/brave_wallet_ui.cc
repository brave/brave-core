/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <memory>
#include <string>

#include "brave/browser/ui/webui/brave_wallet_ui.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_message_handler.h"

using content::WebUIMessageHandler;

namespace {

class WalletDOMHandler : public WebUIMessageHandler {
 public:
  WalletDOMHandler() {
  }
  ~WalletDOMHandler() override {}

  void Init();
  void RegisterMessages() override;

 private:
  void HandleImportNowRequested(const base::ListValue* args);
  Browser* GetBrowser();
  DISALLOW_COPY_AND_ASSIGN(WalletDOMHandler);
};

void WalletDOMHandler::Init() {
}

void WalletDOMHandler::RegisterMessages() {
}

Browser* WalletDOMHandler::GetBrowser() {
  return chrome::FindBrowserWithWebContents(web_ui()->GetWebContents());
}

void WalletDOMHandler::HandleImportNowRequested(
  const base::ListValue* args) {
  chrome::ShowSettingsSubPageInTabbedBrowser(
    GetBrowser(),
    chrome::kImportDataSubPage);
}

}  // namespace

BraveWalletUI::BraveWalletUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kBraveWalletGenerated,
        kBraveWalletGeneratedSize, IDR_BRAVE_WALLET_HTML) {
  auto handler_owner = std::make_unique<WalletDOMHandler>();
  WalletDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveWalletUI::~BraveWalletUI() {
}
