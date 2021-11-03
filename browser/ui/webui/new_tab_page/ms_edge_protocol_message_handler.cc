// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/ms_edge_protocol_message_handler.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/task/thread_pool.h"
#include "brave/browser/default_protocol_handler_utils_win.h"

namespace {

constexpr wchar_t kMSEdgeProtocol[] = L"microsoft-edge";

}  // namespace

MSEdgeProtocolMessageHandler::MSEdgeProtocolMessageHandler() = default;
MSEdgeProtocolMessageHandler::~MSEdgeProtocolMessageHandler() = default;

void MSEdgeProtocolMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "checkDefaultMSEdgeProtocolHandlerState",
      base::BindRepeating(&MSEdgeProtocolMessageHandler::
                              HandleCheckDefaultMSEdgeProtocolHandlerState,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "setAsDefaultMSEdgeProtocolHandler",
      base::BindRepeating(&MSEdgeProtocolMessageHandler::
                              HandleSetAsDefaultMSEdgeProtocolHandler,
                          base::Unretained(this)));
}

void MSEdgeProtocolMessageHandler::HandleCheckDefaultMSEdgeProtocolHandlerState(
    base::Value::ConstListView args) {
  AllowJavascript();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&IsDefaultProtocolHandlerFor, kMSEdgeProtocol),
      base::BindOnce(&MSEdgeProtocolMessageHandler::OnIsDefaultProtocolHandler,
                     weak_factory_.GetWeakPtr()));
}

void MSEdgeProtocolMessageHandler::HandleSetAsDefaultMSEdgeProtocolHandler(
    base::Value::ConstListView args) {
  AllowJavascript();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&SetDefaultProtocolHandlerFor, kMSEdgeProtocol),
      base::BindOnce(&MSEdgeProtocolMessageHandler::OnSetDefaultProtocolHandler,
                     weak_factory_.GetWeakPtr()));
}

void MSEdgeProtocolMessageHandler::OnIsDefaultProtocolHandler(bool is_default) {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("notify-ms-edge-protocol-default-handler-status",
                      base::Value(is_default));
  }
}

void MSEdgeProtocolMessageHandler::OnSetDefaultProtocolHandler(bool success) {
  if (!success) {
    // TODO(simonhong): Launch system ui as a fallback.
    return;
  }

  if (IsJavascriptAllowed()) {
    FireWebUIListener("notify-ms-edge-protocol-default-handler-status",
                      base::Value(success));
  }
}
