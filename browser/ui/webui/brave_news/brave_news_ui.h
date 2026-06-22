// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_BRAVE_NEWS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_BRAVE_NEWS_UI_H_

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "chrome/browser/ui/webui/top_chrome/untrusted_top_chrome_web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace brave_news {
class BraveNewsController;
}

namespace content {
class BrowserContext;
class WebUI;
}  // namespace content

class BraveNewsUI : public UntrustedTopChromeWebUIController {
 public:
  explicit BraveNewsUI(content::WebUI* web_ui);
  BraveNewsUI(const BraveNewsUI&) = delete;
  BraveNewsUI& operator=(const BraveNewsUI&) = delete;
  ~BraveNewsUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver);

  static constexpr std::string_view GetWebUIName() { return "BraveNewsPanel"; }

 private:
  raw_ptr<brave_news::BraveNewsController> controller_ = nullptr;
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveNewsUIConfig : public DefaultTopChromeWebUIConfig<BraveNewsUI> {
 public:
  BraveNewsUIConfig();

  // DefaultTopChromeWebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEWS_BRAVE_NEWS_UI_H_
