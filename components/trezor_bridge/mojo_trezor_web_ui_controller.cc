/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/trezor_bridge/mojo_trezor_web_ui_controller.h"
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
