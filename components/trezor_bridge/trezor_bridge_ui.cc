/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/trezor_bridge/trezor_bridge_ui.h"

#include "brave/common/webui_url_constants.h"
#include "brave/components/trezor_bridge/trezor_bridge_handler.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/resource/resource_bundle.h"

using content::WebUIMessageHandler;

namespace {

content::WebUIDataSource* CreateTrezorBridgeHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kBraveTrezorBridgeHost);

  source->AddResourcePath("brave.js",
                          IDR_TREZOR_BRIDGE_HTML_BRAVE_JS);
  source->AddResourcePath("main.d598eca52e4331ccfdcb.js",
                          IDR_TREZOR_BRIDGE_HTML_MAIN_JS);
  source->AddResourcePath("assets/trezor-connect/js/brave.js",
                          IDR_TREZOR_BRIDGE_HTML_IFRAME_BRAVE_JS);
  source->AddResourcePath("assets/trezor-connect/js/iframe.24e81bebce2daadbca5c.js",
                          IDR_TREZOR_BRIDGE_HTML_IFRAME_JS);
  source->AddResourcePath("assets/trezor-connect/workers/blockbook-worker.7b7e96fe49afeaf40fd2.js",
                          IDR_TREZOR_BRIDGE_HTML_IFRAME_WORKER_BLOCKBOOK);
  source->AddResourcePath("assets/trezor-connect/workers/ripple-worker.909828998a247167a2b5.js",
                          IDR_TREZOR_BRIDGE_HTML_IFRAME_WORKER_RIPPLE);
  source->AddResourcePath("assets/trezor-connect/workers/shared-connection-worker.36c366be58ccbe52d6ce.js",
                          IDR_TREZOR_BRIDGE_HTML_IFRAME_WORKER_SHARED);
  source->AddResourcePath("assets/trezor-connect/iframe.html",
                          IDR_TREZOR_BRIDGE_HTML_IFRAME);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      "frame-src chrome://trezor-bridge;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome://resources/ chrome://trezor-bridge;");

  source->DisableDenyXFrameOptions();
  source->SetDefaultResource(IDR_TREZOR_BRIDGE_HTML);
  return source;
}

}

TrezorBridgeUI::TrezorBridgeUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui) {
  
  web_ui->AddMessageHandler(std::make_unique<TrezorBridgeHandler>());
  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                CreateTrezorBridgeHTMLSource());
}

TrezorBridgeUI::~TrezorBridgeUI() {
}
