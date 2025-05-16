/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/status_icon.h"

#include <memory>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/strings/string_util_win.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/native_popup_menu.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/tray_menu_model.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/icon_util.h"

namespace brave_vpn {
namespace {
std::unique_ptr<NOTIFYICONDATA> GetIconData(HWND window, UINT uFlags = 0) {
  return std::make_unique<NOTIFYICONDATA>(
      NOTIFYICONDATA{.cbSize = sizeof(NOTIFYICONDATA),
                     .hWnd = window,
                     .uID = 2,  // Use 2 to avoid conflicts with other apps.
                     .uFlags = uFlags});
}
}  // namespace

StatusIcon::StatusIcon(HWND window, UINT message)
    : window_(window), message_id_(message) {}

StatusIcon::~StatusIcon() {
  // Remove our icon
  DeleteIcon();
}

void StatusIcon::HandleClickEvent(const gfx::Point& cursor_pos,
                                  bool left_mouse_click) {
  if (!menu_model_) {
    return;
  }

  // Set our window as the foreground window, so the context menu closes when
  // we click away from it.
  if (!SetForegroundWindow(window_)) {
    return;
  }
  menu_model_->MenuWillShow();
  popup_menu_ = std::make_unique<NativePopupMenu>(menu_model_.get());

  TrackPopupMenuEx(popup_menu_->GetWeakMenuHandle(), TPM_BOTTOMALIGN,
                   cursor_pos.x(), cursor_pos.y(), window_, NULL);
}

void StatusIcon::OnMenuCommand(int index, int event_flags) {
  DCHECK(menu_model_.get());
  if (!menu_model_->delegate()) {
    return;
  }
  ExecuteCommand(menu_model_->GetCommandIdAt(index), event_flags);
}

void StatusIcon::ExecuteCommand(int command_id, int event_flags) {
  menu_model_->ExecuteCommand(command_id, event_flags);
}

void StatusIcon::DeleteIcon() {
  auto delete_data = GetIconData(window_);
  // Delete any previously existing icon.
  Shell_NotifyIcon(NIM_DELETE, delete_data.get());
}

void StatusIcon::AddIcon() {
  auto icon_data = GetIconData(window_, NIF_MESSAGE);
  icon_data->uCallbackMessage = message_id_;
  if (icon_.get()) {
    icon_data->hIcon = icon_.get();
    icon_data->uFlags |= NIF_ICON;
  }
  if (!Shell_NotifyIcon(NIM_ADD, icon_data.get())) {
    LOG(WARNING) << "Unable to re-create status tray icon.";
  }
}

void StatusIcon::ResetIcon() {
  DeleteIcon();
  AddIcon();
}

void StatusIcon::UpdateIcon() {
  auto icon_data = GetIconData(window_, NIF_ICON);
  icon_data->hIcon = icon_.get();
  if (!Shell_NotifyIcon(NIM_MODIFY, icon_data.get())) {
    LOG(WARNING) << "Error setting status tray icon image";
  }
}

void StatusIcon::SetToolTip(const std::u16string& tool_tip) {
  auto icon_data = GetIconData(window_, NIF_TIP);
  UNSAFE_TODO(wcscpy_s(icon_data->szTip, base::as_wcstr(tool_tip)));
  if (!Shell_NotifyIcon(NIM_MODIFY, icon_data.get())) {
    LOG(WARNING) << "Unable to set tooltip for status tray icon";
  }
}

void StatusIcon::UpdateState(const gfx::ImageSkia& image,
                             const std::u16string& tool_tip) {
  bool update_existing = icon_.is_valid();
  icon_ = IconUtil::CreateHICONFromSkBitmap(*image.bitmap());
  if (!update_existing) {
    AddIcon();
  } else {
    UpdateIcon();
  }
  SetToolTip(tool_tip);
}

void StatusIcon::SetContextMenu(std::unique_ptr<TrayMenuModel> menu) {
  menu_model_ = std::move(menu);
}

}  // namespace brave_vpn
