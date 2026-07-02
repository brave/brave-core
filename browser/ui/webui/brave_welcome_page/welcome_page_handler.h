// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_HANDLER_H_

#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_welcome_page {

// Handler for communication with the welcome page front end application.
class WelcomePageHandler : public mojom::WelcomePageHandler {
 public:
  explicit WelcomePageHandler(
      mojo::PendingReceiver<mojom::WelcomePageHandler> receiver);

  WelcomePageHandler(const WelcomePageHandler&) = delete;
  WelcomePageHandler& operator=(const WelcomePageHandler&) = delete;

  ~WelcomePageHandler() override;

  // mojom::WelcomePageHandler:
  void SetWelcomePage(mojo::PendingRemote<mojom::WelcomePage> page) override;

 private:
  mojo::Receiver<mojom::WelcomePageHandler> receiver_;
  mojo::Remote<mojom::WelcomePage> page_;
};

}  // namespace brave_welcome_page

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_HANDLER_H_
