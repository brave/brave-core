/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/deprecated_brave_wallet_ui.h"

#include <memory>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/deprecated_brave_wallet_page/resources/grit/deprecated_brave_wallet_page_generated_map.h"
#include "components/grit/brave_components_resources.h"

DeprecatedBraveWalletUI::DeprecatedBraveWalletUI(content::WebUI* web_ui,
                                                 const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(
      web_ui, name, kDeprecatedBraveWalletPageGenerated,
      kDeprecatedBraveWalletPageGeneratedSize,
      IDR_BRAVE_DEPRECATED_WALLET_HTML, true /*disable_trusted_types_csp*/);
}

DeprecatedBraveWalletUI::~DeprecatedBraveWalletUI() = default;
