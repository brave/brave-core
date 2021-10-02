/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/trezor_bridge_controller.h"

#include <memory>
#include <string>
#include <utility>

#include "base/gtest_prod_util.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "brave/browser/ui/webui/brave_wallet/trezor_bridge/trezor_bridge_ui.h"
#include "content/public/browser/web_contents.h"

namespace {

content::WebContents::CreateParams GetWebContentsCreateParams(
  content::BrowserContext* browser_context) {
    content::WebContents::CreateParams create_params(browser_context);
    create_params.initially_hidden = true;
    return create_params;
  }

}  // namespace

namespace brave_wallet {

TrezorBridgeController::TrezorBridgeController(
    content::BrowserContext* browser_context)
      : web_contents_(content::WebContents::Create(
                GetWebContentsCreateParams(browser_context))) {
}

TrezorBridgeController::~TrezorBridgeController() = default;

mojo::PendingRemote<mojom::TrezorBridgeController>
TrezorBridgeController::MakeRemote() {
  mojo::PendingRemote<mojom::TrezorBridgeController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void TrezorBridgeController::Bind(
    mojo::PendingReceiver<mojom::TrezorBridgeController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

bool TrezorBridgeController::IsUnlocked() {
  return unlocked_ &&
    web_contents()->GetVisibleURL().host() == kBraveTrezorBridgeHost;
}

void TrezorBridgeController::Unlock(UnlockCallback callback) {
  if (!unlocked_) {
    web_contents()->GetController().LoadURL(GURL(kBraveTrezorBridgeURL),
        content::Referrer(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());

    if (TrezorBridgeUI* webui_controller = GetWebUIController())
      webui_controller->set_embedder(weak_ptr_factory_.GetWeakPtr());
  }
  
  std::move(callback).Run(true);
}

void TrezorBridgeController::Unlock() {
  
}

void TrezorBridgeController::GetAccounts(std::vector<std::string> accounts) {

}

TrezorBridgeUI* TrezorBridgeController::GetWebUIController() {
  content::WebUI* const webui = web_contents()->GetWebUI();
  return webui && webui->GetController()
              ? webui->GetController()->template GetAs<TrezorBridgeUI>()
              : nullptr;
}

void TrezorBridgeController::GetAddress(const std::string& path,
                                        GetAddressCallback callback) {
  std::move(callback).Run(true, path);
}

}  // namespace brave_wallet
