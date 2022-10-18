// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_DATA_HANDLER_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_DATA_HANDLER_IMPL_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "brave/components/speedreader/common/speedreader_panel.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

class TabStripModel;

using speedreader::mojom::Theme;

namespace speedreader {
class SpeedreaderTabHelper;
}  // namespace speedreader

class SpeedreaderPanelDataHandlerImpl
    : public speedreader::mojom::PanelDataHandler {
 public:
  SpeedreaderPanelDataHandlerImpl(
      mojo::PendingReceiver<speedreader::mojom::PanelDataHandler> receiver,
      content::WebContents* web_contents);

  SpeedreaderPanelDataHandlerImpl(const SpeedreaderPanelDataHandlerImpl&) =
      delete;
  SpeedreaderPanelDataHandlerImpl& operator=(
      const SpeedreaderPanelDataHandlerImpl&) = delete;

  ~SpeedreaderPanelDataHandlerImpl() override;

  // speedreader::mojom::PanelDatahandler overrides
  void GetTheme(GetThemeCallback callback) override;
  void SetTheme(Theme theme) override;

 private:
  mojo::Receiver<speedreader::mojom::PanelDataHandler> receiver_;
  raw_ptr<speedreader::SpeedreaderTabHelper> speedreader_tab_helper_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_DATA_HANDLER_IMPL_H_
