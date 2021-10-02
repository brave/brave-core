/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/trezor_bridge/trezor_bridge_page_handler.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "brave/browser/ui/webui/brave_wallet/trezor_bridge/trezor_bridge_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "url/gurl.h"

TrezorBridgePageHandler::TrezorBridgePageHandler(
    mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver,
    mojo::PendingRemote<trezor_bridge::mojom::Page> page,
    TrezorBridgeUI* trezor_bridge_ui,
    content::WebUI* web_ui)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      trezor_bridge_ui_(trezor_bridge_ui),
      web_ui_(web_ui),
      web_contents_(web_ui->GetWebContents()) {
}

TrezorBridgePageHandler::~TrezorBridgePageHandler() = default;

void TrezorBridgePageHandler::OnAddressesFetched(const std::vector<std::string>& addresses) {
  DLOG(INFO) << trezor_bridge_ui_;
}

void TrezorBridgePageHandler::OnUnlocked(bool success) {
  DLOG(INFO) << web_ui_;
}