/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_STATUS_ICON_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_STATUS_ICON_H_

#include <windows.h>  // windows.h must be included before shellapi.h

#include <shellapi.h>
#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/win/scoped_gdi_object.h"
#include "ui/gfx/image/image_skia.h"

namespace gfx {
class Point;
}  // namespace gfx

namespace brave_vpn {

class NativePopupMenu;
class TrayMenuModel;

class StatusIcon {
 public:
  StatusIcon(HWND window, UINT message);

  StatusIcon(const StatusIcon&) = delete;
  StatusIcon& operator=(const StatusIcon&) = delete;

  ~StatusIcon();

  // Handles a click event from the user.
  void HandleClickEvent(const gfx::Point& cursor_pos, bool left_button_click);

  void UpdateState(const gfx::ImageSkia& image, const std::u16string& tool_tip);
  void SetContextMenu(std::unique_ptr<TrayMenuModel> menu);
  void OnMenuCommand(int index, int event_flags);
  void ExecuteCommand(int command_id, int event_flags);
  void ResetIcon();

 private:
  void SetImage(const gfx::ImageSkia& image);
  void SetToolTip(const std::u16string& tool_tip);

  void UpdateIcon();
  void AddIcon();
  void DeleteIcon();

  HWND window() const { return window_; }
  UINT message_id() const { return message_id_; }

  // Window used for processing messages from this icon.
  HWND window_;

  // The message identifier used for status icon messages.
  UINT message_id_;

  // The currently-displayed icon for the window.
  base::win::ScopedHICON icon_;
  std::unique_ptr<NativePopupMenu> popup_menu_;
  // Context menu, if any.
  std::unique_ptr<TrayMenuModel> menu_model_;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_STATUS_ICON_H_
