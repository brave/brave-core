// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_INSPECT_BRAVE_NEWS_INSPECT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_INSPECT_BRAVE_NEWS_INSPECT_UI_H_

#include <memory>

#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

class Browser;

class BraveNewsInspectUI : public content::WebUIController {
 public:
  explicit BraveNewsInspectUI(content::WebUI* web_ui, const std::string& host);
  BraveNewsInspectUI(const BraveNewsInspectUI&) = delete;
  BraveNewsInspectUI& operator=(const BraveNewsInspectUI&) = delete;
  ~BraveNewsInspectUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_INSPECT_BRAVE_NEWS_INSPECT_UI_H_
