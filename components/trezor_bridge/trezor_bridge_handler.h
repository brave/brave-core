/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TREZOR_BRIDGE_TREZOR_BRIDGE_HANDLER_H_
#define BRAVE_COMPONENTS_TREZOR_BRIDGE_TREZOR_BRIDGE_HANDLER_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_message_handler.h"

class TrezorBridgeHandler : public content::WebUIMessageHandler {
 public:
  TrezorBridgeHandler();
  ~TrezorBridgeHandler() override;

  void HandleFetchRequest(const base::ListValue* args);

 private:
  // content::WebUIMessageHandler
  void RegisterMessages() override;
  
  base::WeakPtrFactory<TrezorBridgeHandler> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(TrezorBridgeHandler);
};

#endif  // BRAVE_COMPONENTS_TREZOR_BRIDGE_TREZOR_BRIDGE_HANDLER_H_
