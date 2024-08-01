// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_data_handler_impl.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class Browser;
class Profile;

namespace content {
class BrowserContext;
}

class SpeedreaderToolbarUI : public TopChromeWebUIController,
                             public speedreader::mojom::ToolbarFactory {
 public:
  explicit SpeedreaderToolbarUI(content::WebUI* web_ui);
  ~SpeedreaderToolbarUI() override;
  SpeedreaderToolbarUI(const SpeedreaderToolbarUI&) = delete;
  SpeedreaderToolbarUI& operator=(const SpeedreaderToolbarUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<speedreader::mojom::ToolbarFactory> receiver);

  static constexpr std::string GetWebUIName() { return "SpeedreaderPanel"; }

 private:
  void CreateInterfaces(
      mojo::PendingReceiver<speedreader::mojom::ToolbarDataHandler>
          toolbar_data_handler,
      mojo::PendingRemote<speedreader::mojom::ToolbarEventsHandler>
          toolbar_events_handler) override;

  mojo::Receiver<speedreader::mojom::ToolbarFactory> toolbar_factory_{this};
  std::unique_ptr<SpeedreaderToolbarDataHandlerImpl> toolbar_data_handler_;

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class SpeedreaderToolbarUIConfig
    : public DefaultTopChromeWebUIConfig<SpeedreaderToolbarUI> {
 public:
  SpeedreaderToolbarUIConfig();

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_UI_H_
