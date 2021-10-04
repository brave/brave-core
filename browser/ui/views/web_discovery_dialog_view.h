/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WEB_DISCOVERY_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WEB_DISCOVERY_DIALOG_VIEW_H_

#include <memory>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class Checkbox;
}  // namespace views

class Browser;
class PrefService;

class WebDiscoveryDialogView : public views::DialogDelegateView {
 public:
  METADATA_HEADER(WebDiscoveryDialogView);

  WebDiscoveryDialogView(Browser* browser, PrefService* prefs);
  ~WebDiscoveryDialogView() override;

  WebDiscoveryDialogView(const WebDiscoveryDialogView&) = delete;
  WebDiscoveryDialogView& operator=(const WebDiscoveryDialogView&) = delete;

  // views::DialogDelegateView overrides:
  views::ClientView* CreateClientView(views::Widget* widget) override;
  bool ShouldShowCloseButton() const override;

 private:
  void CreateChildViews();
  void OnAcceptButtonClicked();
  void OnWindowClosing();
  void OnLearnMoreClicked();

  views::Checkbox* dont_ask_again_checkbox_ = nullptr;
  Browser* browser_ = nullptr;
  PrefService* prefs_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WEB_DISCOVERY_DIALOG_VIEW_H_
