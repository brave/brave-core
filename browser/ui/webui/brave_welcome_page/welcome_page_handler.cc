// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_handler.h"

#include <utility>

namespace brave_welcome_page {

WelcomePageHandler::WelcomePageHandler(
    mojo::PendingReceiver<mojom::WelcomePageHandler> receiver)
    : receiver_(this, std::move(receiver)) {}

WelcomePageHandler::~WelcomePageHandler() = default;

void WelcomePageHandler::SetWelcomePage(
    mojo::PendingRemote<mojom::WelcomePage> page) {
  page_.Bind(std::move(page));
}

}  // namespace brave_welcome_page
