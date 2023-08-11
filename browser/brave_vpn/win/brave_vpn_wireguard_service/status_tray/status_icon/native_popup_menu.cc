/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/native_popup_menu.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "ui/base/models/menu_model.h"

namespace brave_vpn {

struct NativePopupMenu::ItemData {
  // The Windows API requires that whoever creates the menus must own the
  // strings used for labels, and keep them around for the lifetime of the
  // created menu.
  std::u16string label;
};

NativePopupMenu::NativePopupMenu(ui::MenuModel* model)
    : model_(model), popup_menu_(nullptr) {
  Build();
}

NativePopupMenu::~NativePopupMenu() {
  items_.clear();
}

HMENU NativePopupMenu::GetWeakMenuHandle() {
  return popup_menu_.get();
}

void NativePopupMenu::Build() {
  ResetNativeMenu();
  items_.clear();

  for (size_t model_index = 0; model_index < model_->GetItemCount();
       ++model_index) {
    if (model_->GetTypeAt(model_index) == ui::MenuModel::TYPE_SEPARATOR) {
      AddSeparatorItemAt(model_index);
    } else {
      AddMenuItemAt(model_index);
    }
  }
}

void NativePopupMenu::AddSeparatorItemAt(size_t menu_index) {
  MENUITEMINFO mii = {
      .cbSize = sizeof(mii), .fMask = MIIM_FTYPE, .fType = MFT_SEPARATOR};
  items_.insert(items_.begin() + static_cast<ptrdiff_t>(menu_index),
                std::make_unique<ItemData>());
  InsertMenuItem(GetWeakMenuHandle(), static_cast<UINT>(menu_index), TRUE,
                 &mii);
}

void NativePopupMenu::AddMenuItemAt(size_t menu_index) {
  MENUITEMINFO mii = {
      .cbSize = sizeof(mii),
      .fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_STATE,
      .fType = MFT_STRING,
  };

  std::unique_ptr<ItemData> item_data = std::make_unique<ItemData>();
  item_data->label = model_->GetLabelAt(menu_index);
  mii.dwItemData = reinterpret_cast<ULONG_PTR>(item_data.get());

  items_.insert(items_.begin() + static_cast<ptrdiff_t>(menu_index),
                std::move(item_data));
  // MIIM_STRING
  mii.dwTypeData = base::as_writable_wcstr(items_[menu_index]->label);
  // MIIM_STATE
  mii.fState = model_->IsEnabledAt(menu_index) ? MFS_ENABLED : MFS_DISABLED;
  ;

  InsertMenuItem(GetWeakMenuHandle(), menu_index, TRUE, &mii);
}

void NativePopupMenu::ResetNativeMenu() {
  popup_menu_.reset(CreatePopupMenu());
  MENUINFO mi = {.cbSize = sizeof(mi),
                 .fMask = MIM_STYLE | MIM_MENUDATA,
                 .dwStyle = MNS_NOTIFYBYPOS,
                 .dwMenuData = reinterpret_cast<ULONG_PTR>(this)};
  SetMenuInfo(GetWeakMenuHandle(), &mi);
}

}  // namespace brave_vpn
