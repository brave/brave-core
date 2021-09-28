/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TREZOR_BRIDGE_TREZOR_BRIDGE_HANDLER_H_
#define BRAVE_COMPONENTS_TREZOR_BRIDGE_TREZOR_BRIDGE_HANDLER_H_

#include <list>
#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class TrezorBridgeHandler : public content::WebUIMessageHandler {
 public:
  TrezorBridgeHandler(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~TrezorBridgeHandler() override;

  void HandleFetchRequest(const base::ListValue* args);
  void SetRequestCallbackForTesting(
      base::OnceCallback<void(const base::Value&)> callback) {
    callback_for_testing_ = std::move(callback);
  }

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  // content::WebUIMessageHandler
  void RegisterMessages() override;

  void OnRequestResponse(SimpleURLLoaderList::iterator iter,
                         base::Value callback_id,
                         std::unique_ptr<std::string> response_body);
  void RespondRequestCallback(const base::Value& callback_id,
                              bool success,
                              const std::string& text,
                              const std::string& status_text);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  base::OnceCallback<void(const base::Value&)> callback_for_testing_;
  base::WeakPtrFactory<TrezorBridgeHandler> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(TrezorBridgeHandler);
};

#endif  // BRAVE_COMPONENTS_TREZOR_BRIDGE_TREZOR_BRIDGE_HANDLER_H_
