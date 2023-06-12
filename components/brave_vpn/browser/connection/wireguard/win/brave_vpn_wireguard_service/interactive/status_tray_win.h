/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_STATUS_TRAY_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_STATUS_TRAY_WIN_H_

#include <windows.h>

#include <memory>
#include <string>
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/common/scoped_hwnd.h"
#include "ui/gfx/image/image_skia.h"

class StatusIconWin;

class StatusTrayWin {
 public:
  StatusTrayWin();

  StatusTrayWin(const StatusTrayWin&) = delete;
  StatusTrayWin& operator=(const StatusTrayWin&) = delete;

  ~StatusTrayWin();

  static bool IconWindowExists();

  StatusIconWin* CreateStatusIcon(const gfx::ImageSkia& image,
                                  const std::u16string& tool_tip);

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
  brave::win::ScopedHWND window_;

  // The message ID of the "TaskbarCreated" message, sent to us when we need to
  // reset our status icons.
  UINT taskbar_created_message_;

  // Manages changes performed on a background thread to manipulate visibility
  // of notification icons.
  std::unique_ptr<StatusIconWin> status_icon_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_STATUS_TRAY_WIN_H_
