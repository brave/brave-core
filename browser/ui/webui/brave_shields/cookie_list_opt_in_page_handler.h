/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_PAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/core/common/cookie_list_opt_in.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class Profile;

class CookieListOptInPageHandler
    : public brave_shields::mojom::CookieListOptInPageHandler {
 public:
  CookieListOptInPageHandler(
      mojo::PendingReceiver<brave_shields::mojom::CookieListOptInPageHandler>
          receiver,
      base::WeakPtr<TopChromeWebUIController::Embedder> embedder,
      Profile* profile);

  CookieListOptInPageHandler(const CookieListOptInPageHandler&) = delete;
  CookieListOptInPageHandler& operator=(const CookieListOptInPageHandler&) =
      delete;

  ~CookieListOptInPageHandler() override;

  // brave_shields::mojom::CookieListOptInPageHandler:
  void ShowUI() override;
  void CloseUI() override;
  void EnableFilter() override;

  void OnUINoClicked() override;
  void OnUIYesClicked() override;

 private:
  mojo::Receiver<brave_shields::mojom::CookieListOptInPageHandler> receiver_;
  base::WeakPtr<TopChromeWebUIController::Embedder> embedder_;
  raw_ptr<Profile> profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_PAGE_HANDLER_H_
