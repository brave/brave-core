/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet_ui.h"

#include <memory>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_page_generated_map.h"
#include "components/grit/brave_components_resources.h"

BraveWalletUI::BraveWalletUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(
      web_ui, name, kBraveWalletPageGenerated, kBraveWalletPageGeneratedSize,
      IDR_BRAVE_WALLET_HTML, true /*disable_trusted_types_csp*/);
}

BraveWalletUI::~BraveWalletUI() = default;
