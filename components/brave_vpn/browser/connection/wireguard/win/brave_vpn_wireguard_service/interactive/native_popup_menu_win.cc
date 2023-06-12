/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/native_popup_menu_win.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"

struct NativePopupMenuWin::ItemData {
  // The Windows API requires that whoever creates the menus must own the
  // strings used for labels, and keep them around for the lifetime of the
  // created menu. So be it.
  std::u16string label;

  // The index of the item within the menu's model.
  size_t model_index;
};

NativePopupMenuWin::NativePopupMenuWin(ui::MenuModel* model)
    : model_(model), popup_menu_(nullptr), first_item_index_(0) {
  Rebuild();
}

NativePopupMenuWin::~NativePopupMenuWin() {
  items_.clear();
}

HMENU NativePopupMenuWin::GetWeakMenuHandle() {
  return popup_menu_.get();
}

void NativePopupMenuWin::Rebuild() {
  ResetNativeMenu();
  items_.clear();

  for (size_t model_index = 0; model_index < model_->GetItemCount();
       ++model_index) {
    size_t menu_index = model_index + first_item_index_;
    if (model_->GetTypeAt(model_index) == ui::MenuModel::TYPE_SEPARATOR) {
      AddSeparatorItemAt(menu_index, model_index);
    } else {
      AddMenuItemAt(menu_index, model_index);
    }
  }
}

void NativePopupMenuWin::AddSeparatorItemAt(size_t menu_index,
                                            size_t model_index) {
  MENUITEMINFO mii = {0};
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_FTYPE;
  mii.fType = MFT_SEPARATOR;
  // Insert a dummy entry into our label list so we can index directly into it
  // using item indices if need be.
  items_.insert(items_.begin() + static_cast<ptrdiff_t>(model_index),
                std::make_unique<ItemData>());
  InsertMenuItem(GetWeakMenuHandle(), static_cast<UINT>(menu_index), TRUE,
                 &mii);
}

void NativePopupMenuWin::AddMenuItemAt(size_t menu_index, size_t model_index) {
  MENUITEMINFO mii = {0};
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA;
  if (!model_->HasIcons()) {
    mii.fType = MFT_STRING;
  } else {
    mii.fType = MFT_OWNERDRAW;
  }

  std::unique_ptr<ItemData> item_data = std::make_unique<ItemData>();
  item_data->label = model_->GetLabelAt(model_index);
  item_data->model_index = model_index;
  mii.dwItemData = reinterpret_cast<ULONG_PTR>(item_data.get());

  items_.insert(items_.begin() + static_cast<ptrdiff_t>(model_index),
                std::move(item_data));

  mii.fMask |= MIIM_STRING;
  mii.dwTypeData = base::as_writable_wcstr(items_[model_index]->label);

  UINT state = model_->IsEnabledAt(model_index) ? MFS_ENABLED : MFS_DISABLED;
  if (model_->IsItemCheckedAt(model_index)) {
    state |= MFS_CHECKED;
  }
  mii.fMask |= MIIM_STATE;
  mii.fState = state;

  InsertMenuItem(GetWeakMenuHandle(), menu_index, TRUE, &mii);
}

void NativePopupMenuWin::ResetNativeMenu() {
  popup_menu_.reset(CreatePopupMenu());
  MENUINFO mi = {0};
  mi.cbSize = sizeof(mi);
  mi.fMask = MIM_STYLE | MIM_MENUDATA;
  mi.dwStyle = MNS_NOTIFYBYPOS;
  mi.dwMenuData = reinterpret_cast<ULONG_PTR>(this);
  SetMenuInfo(GetWeakMenuHandle(), &mi);
}
