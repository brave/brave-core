/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_NATIVE_POPUP_MENU_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_NATIVE_POPUP_MENU_WIN_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/common/scoped_hmenu.h"
#include "ui/base/models/menu_model.h"

class NativePopupMenuWin {
 public:
  explicit NativePopupMenuWin(ui::MenuModel* model);

  NativePopupMenuWin(const NativePopupMenuWin&) = delete;
  NativePopupMenuWin& operator=(const NativePopupMenuWin&) = delete;
  ~NativePopupMenuWin();

  HMENU GetWeakMenuHandle();

 private:
  void AddMenuItemAt(size_t menu_index, size_t model_index);
  void AddSeparatorItemAt(size_t menu_index, size_t model_index);
  void ResetNativeMenu();
  void Rebuild();

  // An object that collects all of the data associated with an individual menu
  // item.
  struct ItemData;
  std::vector<std::unique_ptr<ItemData>> items_;

  // Our attached model and delegate.
  raw_ptr<ui::MenuModel> model_;
  brave::win::ScopedHMENU popup_menu_;
  // The index of the first item in the model in the menu.
  size_t first_item_index_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_NATIVE_POPUP_MENU_WIN_H_
