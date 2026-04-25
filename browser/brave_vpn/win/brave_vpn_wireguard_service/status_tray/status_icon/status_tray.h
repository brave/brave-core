/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_STATUS_TRAY_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_STATUS_TRAY_H_

#include <windows.h>

#include <memory>
#include <string>

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/scoped_hwnd.h"
#include "ui/gfx/image/image_skia.h"

namespace brave_vpn {

class StatusIcon;

class StatusTray {
 public:
  StatusTray();

  StatusTray(const StatusTray&) = delete;
  StatusTray& operator=(const StatusTray&) = delete;

  ~StatusTray();

  void CreateStatusIcon(const gfx::ImageSkia& image,
                        const std::u16string& tool_tip);
  StatusIcon* GetStatusIcon();

 private:
  // Static callback invoked when a message comes in to our messaging window.
  static LRESULT CALLBACK WndProcStatic(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam);
  LRESULT CALLBACK WndProc(HWND hwnd,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam);

  // The window class of |window_|.
  ATOM atom_;

  // The handle of the module that contains the window procedure of |window_|.
  HMODULE instance_;

  // The window used for processing events.
  win::ScopedHWND window_;

  // The message ID of the "TaskbarCreated" message, sent to us when we need to
  // reset our status icons.
  UINT taskbar_created_message_;

  // The message ID of the "CustomTrayMessage" message, sent to us when we need
  // to execute status icon commands.
  UINT custom_tray_message_;

  std::unique_ptr<StatusIcon> status_icon_;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_STATUS_TRAY_H_
