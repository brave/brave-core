// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_NEW_TAB_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_NEW_TAB_PAGE_UI_H_

#include <memory>

#include "brave/browser/ui/webui/brave_new_tab/new_tab_page.mojom.h"
#include "chrome/common/webui_url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace content {
class WebUI;
}

namespace brave_new_tab {

// The Web UI controller for the Brave new tab page.
class NewTabPageUI : public ui::MojoWebUIController {
 public:
  explicit NewTabPageUI(content::WebUI* web_ui);
  ~NewTabPageUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::NewTabPageHandler> pending_receiver);

 private:
  std::unique_ptr<mojom::NewTabPageHandler> page_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace brave_new_tab

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_NEW_TAB_PAGE_UI_H_
