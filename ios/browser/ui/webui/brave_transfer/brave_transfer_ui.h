// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_UI_H_

#include <memory>

#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

class GURL;

namespace web {
class WebUIIOS;
}  // namespace web

namespace brave_transfer {

class BraveTransferPageHandler;

// brave://transfer on iOS. Serves the same page as desktop; the page handler
// forwards to the Swift engine. Registered for regular profiles only, and
// only while the BraveTransfer feature is enabled.
class BraveTransferUI : public web::WebUIIOSController,
                        public mojom::PageHandlerFactory {
 public:
  BraveTransferUI(web::WebUIIOS* web_ui, const GURL& url);
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
};

}  // namespace brave_transfer

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_UI_H_
