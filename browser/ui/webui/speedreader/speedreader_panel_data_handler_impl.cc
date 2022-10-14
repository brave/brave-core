// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_panel_data_handler_impl.h"

#include <utility>

#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"

SpeedreaderPanelDataHandlerImpl::SpeedreaderPanelDataHandlerImpl(
    mojo::PendingReceiver<speedreader::mojom::PanelDataHandler> receiver,
    content::WebContents* web_contents)
    : receiver_(this, std::move(receiver)) {
  DCHECK(web_contents);

  if (!web_contents)
    return;
  speedreader_tab_helper_ =
      speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
}

SpeedreaderPanelDataHandlerImpl::~SpeedreaderPanelDataHandlerImpl() = default;

void SpeedreaderPanelDataHandlerImpl::GetTheme(GetThemeCallback callback) {
  DCHECK(speedreader_tab_helper_);
  std::move(callback).Run(speedreader_tab_helper_->GetTheme());
}

void SpeedreaderPanelDataHandlerImpl::SetTheme(Theme theme) {
  DCHECK(speedreader_tab_helper_);
  speedreader_tab_helper_->SetTheme(theme);
}

void SpeedreaderPanelDataHandlerImpl::GetFontFamily(
    GetFontFamilyCallback callback) {
  DCHECK(speedreader_tab_helper_);
  std::move(callback).Run(speedreader_tab_helper_->GetFontFamily());
}

void SpeedreaderPanelDataHandlerImpl::SetFontFamily(FontFamily font) {
  DCHECK(speedreader_tab_helper_);
  speedreader_tab_helper_->SetFontFamily(font);
}

void SpeedreaderPanelDataHandlerImpl::GetFontSize(
    GetFontSizeCallback callback) {
  DCHECK(speedreader_tab_helper_);
  std::move(callback).Run(speedreader_tab_helper_->GetFontSize());
}

void SpeedreaderPanelDataHandlerImpl::SetFontSize(FontSize size) {
  DCHECK(speedreader_tab_helper_);
  speedreader_tab_helper_->SetFontSize(size);
}

void SpeedreaderPanelDataHandlerImpl::GetContentStyle(
    GetContentStyleCallback callback) {
  DCHECK(speedreader_tab_helper_);
  std::move(callback).Run(speedreader_tab_helper_->GetContentStyle());
}

void SpeedreaderPanelDataHandlerImpl::SetContentStyle(ContentStyle style) {
  DCHECK(speedreader_tab_helper_);
  speedreader_tab_helper_->SetContentStyle(style);
}

void SpeedreaderPanelDataHandlerImpl::GetCurrentSiteURL(
    GetCurrentSiteURLCallback callback) {
  DCHECK(speedreader_tab_helper_);
  std::move(callback).Run(speedreader_tab_helper_->GetCurrentSiteURL());
}

void SpeedreaderPanelDataHandlerImpl::IsEnabled(IsEnabledCallback callback) {
  DCHECK(speedreader_tab_helper_);
  std::move(callback).Run(speedreader_tab_helper_->IsEnabledForSite());
}

void SpeedreaderPanelDataHandlerImpl::SetEnabled(bool on) {
  DCHECK(speedreader_tab_helper_);
  speedreader_tab_helper_->MaybeToggleEnabledForSite(on);
}
