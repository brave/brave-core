// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_REFRESH_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_REFRESH_HANDLER_H_

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class BraveNewTabPageRefreshHandler
    : public brave_new_tab_page_refresh::mojom::NewTabPageHandler {
 public:
  explicit BraveNewTabPageRefreshHandler(
      mojo::PendingReceiver<
          brave_new_tab_page_refresh::mojom::NewTabPageHandler> receiver);

  ~BraveNewTabPageRefreshHandler() override;

  // brave_new_tab_page_refresh::mojom::NewTabPageHandler:
  void SetNewTabPage(
      mojo::PendingRemote<brave_new_tab_page_refresh::mojom::NewTabPage> page)
      override;

 private:
  mojo::Receiver<brave_new_tab_page_refresh::mojom::NewTabPageHandler>
      receiver_;
  mojo::Remote<brave_new_tab_page_refresh::mojom::NewTabPage> page_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_REFRESH_HANDLER_H_
