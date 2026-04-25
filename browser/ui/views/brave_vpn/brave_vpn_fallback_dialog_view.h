/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_VPN_BRAVE_VPN_FALLBACK_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_VPN_BRAVE_VPN_FALLBACK_DIALOG_VIEW_H_

#include "ui/views/window/dialog_delegate.h"

class Browser;
class PrefService;

namespace views {
class Checkbox;
}

namespace brave_vpn {

class BraveVpnFallbackDialogView : public views::DialogDelegateView {
  METADATA_HEADER(BraveVpnFallbackDialogView, views::DialogDelegateView)
 public:

  static void Show(Browser* browser);

  BraveVpnFallbackDialogView(const BraveVpnFallbackDialogView&) = delete;
  BraveVpnFallbackDialogView& operator=(const BraveVpnFallbackDialogView&) =
      delete;

 private:
  explicit BraveVpnFallbackDialogView(Browser* browser);
  ~BraveVpnFallbackDialogView() override;

  void OnAccept();
  void OnClosing();

  // views::DialogDelegate overrides:
  ui::mojom::ModalType GetModalType() const override;
  bool ShouldShowCloseButton() const override;
  bool ShouldShowWindowTitle() const override;

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<views::Checkbox> dont_ask_again_checkbox_ = nullptr;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_VPN_BRAVE_VPN_FALLBACK_DIALOG_VIEW_H_
