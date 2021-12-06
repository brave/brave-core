// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_MS_EDGE_PROTOCOL_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_MS_EDGE_PROTOCOL_MESSAGE_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "base/win/registry.h"
#include "content/public/browser/web_ui_message_handler.h"

class MSEdgeProtocolMessageHandler : public content::WebUIMessageHandler {
 public:
  static bool CanSetDefaultMSEdgeProtocolHandler();

  MSEdgeProtocolMessageHandler();
  ~MSEdgeProtocolMessageHandler() override;
  MSEdgeProtocolMessageHandler(const MSEdgeProtocolMessageHandler&) = delete;
  MSEdgeProtocolMessageHandler& operator=(const MSEdgeProtocolMessageHandler&) =
      delete;

 private:
  // content::WebUIMessageHandler overrides:
  void RegisterMessages() override;

  void HandleCheckDefaultMSEdgeProtocolHandlerState(
      base::Value::ConstListView args);
  void HandleSetAsDefaultMSEdgeProtocolHandler(base::Value::ConstListView args);

  void OnIsDefaultProtocolHandler(bool is_default);
  void OnSetDefaultProtocolHandler(bool success);

  void CheckMSEdgeProtocolDefaultHandlerState();

  // Watch ms-edge UserChoice reg change.
  void StartWatching();
  void OnRegValChanged();
  void LaunchSystemDialog();

  base::win::RegKey user_choice_key_;
  base::WeakPtrFactory<MSEdgeProtocolMessageHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_MS_EDGE_PROTOCOL_MESSAGE_HANDLER_H_
