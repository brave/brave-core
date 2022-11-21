// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_DATA_HANDLER_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_DATA_HANDLER_IMPL_H_

#include "brave/components/speedreader/common/speedreader_panel.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

using speedreader::mojom::ContentStyle;
using speedreader::mojom::FontFamily;
using speedreader::mojom::FontSize;
using speedreader::mojom::Theme;

class Browser;

namespace speedreader {
class SpeedreaderTabHelper;
}  // namespace speedreader

class SpeedreaderPanelDataHandlerImpl
    : public speedreader::mojom::PanelDataHandler {
 public:
  SpeedreaderPanelDataHandlerImpl(
      mojo::PendingReceiver<speedreader::mojom::PanelDataHandler> receiver,
      Browser* browser);
  SpeedreaderPanelDataHandlerImpl(const SpeedreaderPanelDataHandlerImpl&) =
      delete;
  SpeedreaderPanelDataHandlerImpl& operator=(
      const SpeedreaderPanelDataHandlerImpl&) = delete;

  ~SpeedreaderPanelDataHandlerImpl() override;

  // speedreader::mojom::PanelDataHandler overrides
  void GetTheme(GetThemeCallback callback) override;
  void SetTheme(Theme theme) override;

  void GetFontFamily(GetFontFamilyCallback callback) override;
  void SetFontFamily(FontFamily font) override;

  void GetFontSize(GetFontSizeCallback callback) override;
  void SetFontSize(FontSize size) override;

  void GetContentStyle(GetContentStyleCallback callback) override;
  void SetContentStyle(ContentStyle style) override;

  void GetCurrentSiteURL(GetCurrentSiteURLCallback callback) override;

  void IsEnabled(IsEnabledCallback callback) override;
  void SetEnabled(bool on) override;

  void GetSiteSettings(GetSiteSettingsCallback callback) override;

 private:
  speedreader::SpeedreaderTabHelper* GetSpeedreaderTabHelper();
  void UpdateSiteSettings();

  mojo::Receiver<speedreader::mojom::PanelDataHandler> receiver_;

  raw_ptr<Browser> browser_;
  speedreader::mojom::SiteSettings site_settings_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_DATA_HANDLER_IMPL_H_
