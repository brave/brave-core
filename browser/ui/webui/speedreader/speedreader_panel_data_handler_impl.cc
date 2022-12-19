// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_panel_data_handler_impl.h"

#include <utility>

#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

SpeedreaderPanelDataHandlerImpl::SpeedreaderPanelDataHandlerImpl(
    mojo::PendingReceiver<speedreader::mojom::PanelDataHandler> receiver,
    Browser* browser)
    : receiver_(this, std::move(receiver)), browser_(browser) {
  if (!browser)
    return;
  UpdateSiteSettings();
}

SpeedreaderPanelDataHandlerImpl::~SpeedreaderPanelDataHandlerImpl() = default;

void SpeedreaderPanelDataHandlerImpl::GetTheme(GetThemeCallback callback) {
  std::move(callback).Run(GetSpeedreaderTabHelper()->GetTheme());
}

void SpeedreaderPanelDataHandlerImpl::SetTheme(Theme theme) {
  GetSpeedreaderTabHelper()->SetTheme(theme);
  UpdateSiteSettings();
}

void SpeedreaderPanelDataHandlerImpl::GetFontFamily(
    GetFontFamilyCallback callback) {
  std::move(callback).Run(GetSpeedreaderTabHelper()->GetFontFamily());
}

void SpeedreaderPanelDataHandlerImpl::SetFontFamily(FontFamily font) {
  GetSpeedreaderTabHelper()->SetFontFamily(font);
  UpdateSiteSettings();
}

void SpeedreaderPanelDataHandlerImpl::GetFontSize(
    GetFontSizeCallback callback) {
  std::move(callback).Run(GetSpeedreaderTabHelper()->GetFontSize());
}

void SpeedreaderPanelDataHandlerImpl::SetFontSize(FontSize size) {
  GetSpeedreaderTabHelper()->SetFontSize(size);
  UpdateSiteSettings();
}

void SpeedreaderPanelDataHandlerImpl::GetContentStyle(
    GetContentStyleCallback callback) {
  std::move(callback).Run(GetSpeedreaderTabHelper()->GetContentStyle());
}

void SpeedreaderPanelDataHandlerImpl::SetContentStyle(ContentStyle style) {
  GetSpeedreaderTabHelper()->SetContentStyle(style);
  UpdateSiteSettings();
}

void SpeedreaderPanelDataHandlerImpl::GetCurrentSiteURL(
    GetCurrentSiteURLCallback callback) {
  std::move(callback).Run(GetSpeedreaderTabHelper()->GetCurrentSiteURL());
}

void SpeedreaderPanelDataHandlerImpl::IsEnabled(IsEnabledCallback callback) {
  std::move(callback).Run(GetSpeedreaderTabHelper()->IsEnabledForSite());
}

void SpeedreaderPanelDataHandlerImpl::SetEnabled(bool on) {
  GetSpeedreaderTabHelper()->MaybeToggleEnabledForSite(on);
  UpdateSiteSettings();
}

speedreader::SpeedreaderTabHelper*
SpeedreaderPanelDataHandlerImpl::GetSpeedreaderTabHelper() {
  DCHECK(browser_);
  auto* tab_helper = speedreader::SpeedreaderTabHelper::FromWebContents(
      browser_->tab_strip_model()->GetActiveWebContents());
  DCHECK(tab_helper);
  return tab_helper;
}

void SpeedreaderPanelDataHandlerImpl::UpdateSiteSettings() {
  site_settings_.is_enabled = GetSpeedreaderTabHelper()->IsEnabledForSite();
  site_settings_.host = GetSpeedreaderTabHelper()->GetCurrentSiteURL();
  site_settings_.theme = GetSpeedreaderTabHelper()->GetTheme();
  site_settings_.contentStyle = GetSpeedreaderTabHelper()->GetContentStyle();
  site_settings_.fontFamily = GetSpeedreaderTabHelper()->GetFontFamily();
  site_settings_.fontSize = GetSpeedreaderTabHelper()->GetFontSize();
}

void SpeedreaderPanelDataHandlerImpl::GetSiteSettings(
    GetSiteSettingsCallback callback) {
  std::move(callback).Run(site_settings_.Clone());
}
