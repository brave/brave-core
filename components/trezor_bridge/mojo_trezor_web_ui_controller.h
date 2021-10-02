/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TREZOR_BRIDGE_MOJO_TREZOR_WEB_UI_CONTROLLER_H_
#define BRAVE_COMPONENTS_TREZOR_BRIDGE_MOJO_TREZOR_WEB_UI_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "brave/components/trezor_bridge/trezor_bridge.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class WebUI;
}  // namespace content

class MojoTrezorWebUIController : public ui::MojoWebUIController,
                                  public trezor_bridge::mojom::PageHandlerFactory {
 public:
  class Embedder {
   public:
    virtual void Unlock() = 0;
    virtual void GetAccounts(std::vector<std::string>) = 0;
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


  void set_embedder(base::WeakPtr<Embedder> embedder) { embedder_ = embedder; }
  base::WeakPtr<Embedder> embedder() { return embedder_; }

 private:
  base::WeakPtr<Embedder> embedder_;
  WEB_UI_CONTROLLER_TYPE_DECL();
  
  
  mojo::Receiver<trezor_bridge::mojom::PageHandlerFactory> page_factory_receiver_{
      this};
};

#endif  // BRAVE_COMPONENTS_TREZOR_BRIDGE_MOJO_TREZOR_WEB_UI_CONTROLLER_H_
