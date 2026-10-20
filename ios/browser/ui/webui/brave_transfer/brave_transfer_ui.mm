// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_transfer/brave_transfer_ui.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_transfer/resources/page/grit/brave_transfer_generated_map.h"
#include "brave/ios/browser/ui/webui/brave_transfer/brave_transfer_page_handler.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "url/gurl.h"

namespace brave_transfer {

BraveTransferUI::BraveTransferUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.GetHost()) {
  BraveWebUIIOSDataSource* source = brave::CreateAndAddWebUIDataSource(
      web_ui, url.host(), kBraveTransferGenerated, IDR_BRAVE_TRANSFER_HTML);

  source->AddLocalizedStrings(webui::kBraveTransferStrings);
  source->AddBoolean("isIOS", true);
  source->AddBoolean("isMobile", true);

  // The page bundle does not create Trusted Types policies.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome://resources;");

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&BraveTransferUI::BindInterface,
                          base::Unretained(this)));
}

BraveTransferUI::~BraveTransferUI() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      mojom::PageHandlerFactory::Name_);
}

void BraveTransferUI::BindInterface(
    mojo::PendingReceiver<mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void BraveTransferUI::CreatePageHandler(
    mojo::PendingRemote<mojom::Page> page,
    mojo::PendingReceiver<mojom::PageHandler> handler) {
  // Destroy the previous handler first so the bridge sees its page disconnect
  // before the new one connects.
  page_handler_.reset();
  page_handler_ = std::make_unique<BraveTransferPageHandler>(
      web_ui()->GetWebState(), std::move(handler), std::move(page));
}

}  // namespace brave_transfer
