// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_UI_H_

#include <memory>

#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace content {
class BrowserContext;
class WebUI;
}  // namespace content

namespace brave_transfer {

class BraveTransferPageHandler;
class BraveTransferUI;

class BraveTransferUIConfig
    : public content::DefaultWebUIConfig<BraveTransferUI> {
 public:
  BraveTransferUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBraveTransferHost) {}

  // Only regular profiles, while the BraveTransfer feature is on and the
  // BraveTransferEnabled policy allows it.
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

// brave://transfer: send files, photos, text and links to the user's other
// devices on the local network.
class BraveTransferUI : public ui::MojoWebUIController,
                        public mojom::PageHandlerFactory {
 public:
  explicit BraveTransferUI(content::WebUI* web_ui);
  BraveTransferUI(const BraveTransferUI&) = delete;
  BraveTransferUI& operator=(const BraveTransferUI&) = delete;
  ~BraveTransferUI() override;

  void BindInterface(mojo::PendingReceiver<mojom::PageHandlerFactory> receiver);

 private:
  // mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<mojom::Page> page,
      mojo::PendingReceiver<mojom::PageHandler> handler) override;

  std::unique_ptr<BraveTransferPageHandler> page_handler_;
  mojo::Receiver<mojom::PageHandlerFactory> page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_UI_H_
