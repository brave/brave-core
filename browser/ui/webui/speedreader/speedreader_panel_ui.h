// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/speedreader/speedreader_panel_data_handler_impl.h"
#include "brave/browser/ui/webui/speedreader/speedreader_panel_handler_impl.h"
#include "brave/components/speedreader/common/speedreader_panel.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

class Browser;
class Profile;

class SpeedreaderPanelUI : public ui::MojoBubbleWebUIController,
                           public speedreader::mojom::PanelFactory {
 public:
  SpeedreaderPanelUI(content::WebUI* web_ui, const std::string& name);
  ~SpeedreaderPanelUI() override;
  SpeedreaderPanelUI(const SpeedreaderPanelUI&) = delete;
  SpeedreaderPanelUI& operator=(const SpeedreaderPanelUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<speedreader::mojom::PanelFactory> receiver);

 private:
  void CreateInterfaces(
      mojo::PendingReceiver<speedreader::mojom::PanelHandler> panel_handler,
      mojo::PendingReceiver<speedreader::mojom::PanelDataHandler>
          panel_data_handler) override;

  mojo::Receiver<speedreader::mojom::PanelFactory> panel_factory_{this};

  std::unique_ptr<SpeedreaderPanelHandlerImpl> panel_handler_;
  std::unique_ptr<SpeedreaderPanelDataHandlerImpl> panel_data_handler_;

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_UI_H_
