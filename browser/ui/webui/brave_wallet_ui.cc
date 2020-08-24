/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet_ui.h"

#include <memory>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_webui_url_constants.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {

class WalletDOMHandler : public content::WebUIMessageHandler {
 public:
  WalletDOMHandler();
  ~WalletDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WalletDOMHandler);
};

WalletDOMHandler::WalletDOMHandler() {}

WalletDOMHandler::~WalletDOMHandler() {}

void WalletDOMHandler::RegisterMessages() {
}

}  // namespace

BraveWalletUI::BraveWalletUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kBraveWalletGenerated,
        kBraveWalletGeneratedSize, IDR_BRAVE_WALLET_HTML) {
  web_ui->AddMessageHandler(std::make_unique<WalletDOMHandler>());
}

BraveWalletUI::~BraveWalletUI() {
}
