// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ui/webui/brave_color_change_listener/brave_color_change_handler.h"

namespace ui {

BraveColorChangeHandler::BraveColorChangeHandler(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

BraveColorChangeHandler::~BraveColorChangeHandler() = default;

void BraveColorChangeHandler::SetPage(
    mojo::PendingRemote<color_change_listener::mojom::Page> pending_page) {
  page_.Bind(std::move(pending_page));
}

void BraveColorChangeHandler::OnColorProviderChanged() {
  if (page_) {
    page_->OnColorProviderChanged();
  }
}

}  // namespace ui
