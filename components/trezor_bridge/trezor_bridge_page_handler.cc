/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/trezor_bridge/trezor_bridge_page_handler.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/webui/trezor_bridge/trezor_bridge_ui.h"
#include "url/gurl.h"

TrezorBridgePageHandler::TrezorBridgePageHandler(
    mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver,
    mojo::PendingRemote<trezor_bridge::mojom::Page> page)
    : receiver_(this, std::move(receiver)), page_(std::move(page)) {}

TrezorBridgePageHandler::~TrezorBridgePageHandler() = default;

void TrezorBridgePageHandler::OnAddressesReceived(
    bool success,
    std::vector<trezor_bridge::mojom::HardwareWalletAccountPtr> accounts,
    const std::string& error) {
  DCHECK(subscriber_);
  if (subscriber_)
    subscriber_->OnAddressesReceived(success, std::move(accounts), error);
}

void TrezorBridgePageHandler::OnUnlocked(bool success,
                                         const std::string& error) {
  DCHECK(subscriber_);
  if (subscriber_)
    subscriber_->OnUnlocked(success, error);
}

void TrezorBridgePageHandler::RequestAddresses(
    const std::vector<std::string>& addresses) {
  if (page_)
    page_->requestAddresses(addresses);
}

void TrezorBridgePageHandler::Unlock() {
  if (page_)
    page_->Unlock();
}

void TrezorBridgePageHandler::SetSubscriber(
    base::WeakPtr<MojoTrezorWebUIController::Subscriber> subscriber) {
  subscriber_ = std::move(subscriber);
}