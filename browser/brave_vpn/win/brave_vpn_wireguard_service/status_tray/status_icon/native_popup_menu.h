/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_NATIVE_POPUP_MENU_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_NATIVE_POPUP_MENU_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/scoped_hmenu.h"

namespace ui {
class MenuModel;
}

namespace brave_vpn {

class NativePopupMenu {
 public:
  explicit NativePopupMenu(ui::MenuModel* model);

  NativePopupMenu(const NativePopupMenu&) = delete;
  NativePopupMenu& operator=(const NativePopupMenu&) = delete;
  ~NativePopupMenu();

  HMENU GetWeakMenuHandle();

 private:
  void AddMenuItemAt(size_t menu_index);
  void AddSeparatorItemAt(size_t menu_index);
  void ResetNativeMenu();
  void Build();

  // An object that collects all of the data associated with an individual menu
  // item.
  struct ItemData;
  std::vector<std::unique_ptr<ItemData>> items_;

  raw_ptr<ui::MenuModel> model_;
  win::ScopedHMENU popup_menu_;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_NATIVE_POPUP_MENU_H_
