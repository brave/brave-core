/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_MOJO_TREZOR_WEB_UI_CONTROLLER_H_
#define BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_MOJO_TREZOR_WEB_UI_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/trezor_bridge/browser/trezor_bridge.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace content {
class WebUI;
}  // namespace content

class TrezorBridgePageHandler;

class MojoTrezorWebUIController
    : public ui::MojoWebUIController,
      public trezor_bridge::mojom::PageHandlerFactory {
 public:
  class LibraryController {
   public:
    virtual void RequestAddresses(
        const std::vector<std::string>& addresses) = 0;
    virtual void Unlock() = 0;
  };

  class Subscriber {
   public:
    virtual void OnAddressesReceived(
        bool success,
        std::vector<trezor_bridge::mojom::HardwareWalletAccountPtr> accounts,
        const std::string& error) = 0;
    virtual void OnUnlocked(bool success, const std::string& error) = 0;
  };

  explicit MojoTrezorWebUIController(content::WebUI* contents);
  MojoTrezorWebUIController(const MojoTrezorWebUIController&) = delete;
  MojoTrezorWebUIController& operator=(const MojoTrezorWebUIController&) =
      delete;
  ~MojoTrezorWebUIController() override;

  // Instantiates the implementor of the mojom::PageHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<trezor_bridge::mojom::PageHandlerFactory> receiver);

  base::WeakPtr<Subscriber> subscriber() { return subscriber_; }
  base::WeakPtr<LibraryController> controller() { return controller_; }
  void CreatePageHandler(
      mojo::PendingRemote<trezor_bridge::mojom::Page> page,
      mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver)
      override;
  void SetSubscriber(base::WeakPtr<Subscriber> subscriber);

 protected:
  void SetLibraryController(base::WeakPtr<LibraryController> controller);

 private:
  base::WeakPtr<Subscriber> subscriber_;
  base::WeakPtr<LibraryController> controller_;
  std::unique_ptr<TrezorBridgePageHandler> page_handler_;

  mojo::Receiver<trezor_bridge::mojom::PageHandlerFactory>
      page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_MOJO_TREZOR_WEB_UI_CONTROLLER_H_
