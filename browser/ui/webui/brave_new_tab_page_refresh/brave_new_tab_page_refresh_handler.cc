// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_refresh_handler.h"

#include <utility>

BraveNewTabPageRefreshHandler::BraveNewTabPageRefreshHandler(
    mojo::PendingReceiver<brave_new_tab_page_refresh::mojom::NewTabPageHandler>
        receiver)
    : receiver_(this, std::move(receiver)) {}

BraveNewTabPageRefreshHandler::~BraveNewTabPageRefreshHandler() = default;

void BraveNewTabPageRefreshHandler::SetNewTabPage(
    mojo::PendingRemote<brave_new_tab_page_refresh::mojom::NewTabPage> page) {
  page_.reset();
  page_.Bind(std::move(page));
}
