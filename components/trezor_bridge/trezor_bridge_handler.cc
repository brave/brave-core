/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/logging.h"
#include "brave/components/trezor_bridge/trezor_bridge_handler.h"

TrezorBridgeHandler::TrezorBridgeHandler() {}

TrezorBridgeHandler::~TrezorBridgeHandler() {}

void TrezorBridgeHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "sendFetchRequest",
      base::BindRepeating(&TrezorBridgeHandler::HandleFetchRequest,
                          base::Unretained(this)));
}

void TrezorBridgeHandler::HandleFetchRequest(const base::ListValue* args) {
  LOG(ERROR) << "DebugString:" << args->DebugString();
  ResolveJavascriptCallback(args->GetList()[0], base::Value(false));
}
