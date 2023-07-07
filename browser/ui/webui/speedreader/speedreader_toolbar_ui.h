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
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

class Browser;
class Profile;

class SpeedreaderToolbarUI : public ui::MojoBubbleWebUIController,
                             public speedreader::mojom::ToolbarFactory {
 public:
  SpeedreaderToolbarUI(content::WebUI* web_ui, const std::string& name);
  ~SpeedreaderToolbarUI() override;
  SpeedreaderToolbarUI(const SpeedreaderToolbarUI&) = delete;
  SpeedreaderToolbarUI& operator=(const SpeedreaderToolbarUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<speedreader::mojom::ToolbarFactory> receiver);

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

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_UI_H_
