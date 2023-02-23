// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_

#include <memory>
#include <string>

#include "brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace brave_news {
class BraveNewsController;
}  // namespace brave_news

class BraveNewTabPageHandler;

class BraveNewTabUI : public ui::MojoWebUIController,
                      public brave_new_tab_page::mojom::PageHandlerFactory {
 public:
  BraveNewTabUI(content::WebUI* web_ui, const std::string& name);
  ~BraveNewTabUI() override;
  BraveNewTabUI(const BraveNewTabUI&) = delete;
  BraveNewTabUI& operator=(const BraveNewTabUI&) = delete;

  // Instantiates the implementor of the mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver);

  void BindInterface(
      mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandlerFactory>
          pending_receiver);

 private:
  // new_tab_page::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
      mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
          pending_page_handler) override;

  std::unique_ptr<BraveNewTabPageHandler> page_handler_;
  mojo::Receiver<brave_new_tab_page::mojom::PageHandlerFactory>
      page_factory_receiver_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_
