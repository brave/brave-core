/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/trezor_bridge/trezor_bridge_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/trezor_bridge/resources/grit/trezor_bridge_generated_map.h"
#include "brave/components/trezor_bridge/trezor_bridge_handler.h"
#include "brave/components/trezor_bridge/trezor_bridge_page_handler.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/resources/grit/webui_generated_resources.h"

using content::WebUIMessageHandler;

TrezorBridgeUI::TrezorBridgeUI(content::WebUI* web_ui, const std::string& name)
    : MojoTrezorWebUIController(web_ui) {
  auto* html_source = CreateAndAddWebUIDataSource(
      web_ui, name, kTrezorBridgeGenerated, kTrezorBridgeGeneratedSize,
      IDR_TREZOR_BRIDGE_HTML);
  html_source->AddResourcePath("trezor/iframe.html", IDR_TREZOR_BRIDGE_IFRAME);

  html_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      "frame-src chrome://trezor-bridge;");
  html_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      "frame-ancestors chrome://trezor-bridge;");
  html_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome://resources/ chrome://trezor-bridge ;");
  // Disable DenyXFrame to allow TrezorConnect to create iframes.
  html_source->DisableDenyXFrameOptions();
  auto url_loader = web_ui->GetWebContents()
                        ->GetBrowserContext()
                        ->GetDefaultStoragePartition()
                        ->GetURLLoaderFactoryForBrowserProcess();

  web_ui->AddMessageHandler(
      std::make_unique<TrezorBridgeHandler>(std::move(url_loader)));
}

TrezorBridgeUI::~TrezorBridgeUI() {}

void TrezorBridgeUI::CreatePageHandler(
    mojo::PendingRemote<trezor_bridge::mojom::Page> page,
    mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver) {
  DCHECK(page);
  // TODO
  page_handler_ = std::make_unique<TrezorBridgePageHandler>(std::move(receiver),
                                                            std::move(page));

  SetLibraryController(page_handler_->GetWeakPtr());
}

void TrezorBridgeUI::SetSubscriber(base::WeakPtr<Subscriber> subscriber) {
  DCHECK(page_handler_);
  page_handler_->SetSubscriber(std::move(subscriber));
}