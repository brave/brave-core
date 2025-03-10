// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_ui.h"

#include <utility>

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_data_source.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_refresh_handler.h"

BraveNewTabPageUI::BraveNewTabPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  CreateAndAddBraveNewTabPageDataSource(*web_ui);
}

BraveNewTabPageUI::~BraveNewTabPageUI() = default;

void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<brave_new_tab_page_refresh::mojom::NewTabPageHandler>
        receiver) {
  page_handler_ =
      std::make_unique<BraveNewTabPageRefreshHandler>(std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewTabPageUI)
