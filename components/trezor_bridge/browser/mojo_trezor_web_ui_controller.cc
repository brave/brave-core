/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/trezor_bridge/browser/mojo_trezor_web_ui_controller.h"

#include <memory>
#include <utility>

#include "brave/components/trezor_bridge/browser/trezor_bridge_page_handler.h"
#include "content/public/browser/web_ui.h"

MojoTrezorWebUIController::MojoTrezorWebUIController(content::WebUI* contents)
    : MojoWebUIController(contents, true) {}

MojoTrezorWebUIController::~MojoTrezorWebUIController() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(MojoTrezorWebUIController)

void MojoTrezorWebUIController::BindInterface(
    mojo::PendingReceiver<trezor_bridge::mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void MojoTrezorWebUIController::CreatePageHandler(
    mojo::PendingRemote<trezor_bridge::mojom::Page> page,
    mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver) {
  DCHECK(page);
  page_handler_ = std::make_unique<TrezorBridgePageHandler>(std::move(receiver),
                                                            std::move(page));

  SetLibraryController(page_handler_->GetWeakPtr());
}

void MojoTrezorWebUIController::SetLibraryController(
    base::WeakPtr<LibraryController> controller) {
  controller_ = controller;
}

void MojoTrezorWebUIController::SetSubscriber(
    base::WeakPtr<Subscriber> subscriber) {
  DCHECK(page_handler_);
  page_handler_->SetSubscriber(std::move(subscriber));
}
