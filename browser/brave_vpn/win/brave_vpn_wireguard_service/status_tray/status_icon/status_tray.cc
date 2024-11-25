/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/status_tray.h"

#include <commctrl.h>
#include <wrl/client.h>

#include <utility>

#include "base/files/file_path.h"
#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/status_icon.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/win/hwnd_util.h"

namespace brave_vpn {

namespace {

constexpr UINT kStatusIconMessage = WM_APP + 1;

const base::FilePath::CharType kBraveVpnTaskbarMessageName[] =
    FILE_PATH_LITERAL("TaskbarCreated");

gfx::Point GetCursorScreenPoint() {
  POINT pt;
  ::GetCursorPos(&pt);
  return gfx::Point(pt);
}

}  // namespace

StatusTray::StatusTray() : atom_(0), instance_(NULL) {
  // Register our window class
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kStatusTrayWindowClass,
      &base::win::WrappedWindowProc<StatusTray::WndProcStatic>, 0, 0, 0, NULL,
      NULL, NULL, NULL, NULL, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);
  CHECK(atom_);

  // If the taskbar is re-created after we start up, we have to rebuild all of
  // our icons.
  taskbar_created_message_ = RegisterWindowMessage(kBraveVpnTaskbarMessageName);
  custom_tray_message_ = RegisterWindowMessage(kBraveVpnStatusTrayMessageName);

  // Create an offscreen window for handling messages for the status icons. We
  // create a hidden WS_POPUP window instead of an HWND_MESSAGE window, because
  // only top-level windows such as popups can receive broadcast messages like
  // "TaskbarCreated".
  window_.reset(CreateWindow(MAKEINTATOM(atom_), kStatusTrayWindowName,
                             WS_POPUP, 0, 0, 0, 0, 0, 0, instance_, 0));
  gfx::CheckWindowCreated(window_.get(), ::GetLastError());
  gfx::SetWindowUserData(window_.get(), this);
}

StatusTray::~StatusTray() {
  window_.reset();  // window must be destroyed before unregistering class.
  if (atom_) {
    UnregisterClass(MAKEINTATOM(atom_), instance_);
  }
}

StatusIcon* StatusTray::GetStatusIcon() {
  return status_icon_.get();
}

void StatusTray::CreateStatusIcon(const gfx::ImageSkia& image,
                                  const std::u16string& tooltip) {
  status_icon_ =
      std::make_unique<StatusIcon>(window_.get(), kStatusIconMessage);
  status_icon_->UpdateState(image, tooltip);
}

LRESULT CALLBACK StatusTray::WndProcStatic(HWND hwnd,
                                           UINT message,
                                           WPARAM wparam,
                                           LPARAM lparam) {
  StatusTray* msg_wnd =
      reinterpret_cast<StatusTray*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd) {
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  } else {
    return ::DefWindowProc(hwnd, message, wparam, lparam);
  }
}

LRESULT CALLBACK StatusTray::WndProc(HWND hwnd,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam) {
  if (message == custom_tray_message_) {
    if (status_icon_) {
      status_icon_->ExecuteCommand(wparam, lparam);
    }
    return TRUE;
  } else if (message == taskbar_created_message_) {
    // We need to reset an icon because the taskbar went away.
    if (status_icon_) {
      status_icon_->ResetIcon();
    }

    return TRUE;
  } else if (message == kStatusIconMessage) {
    switch (lparam) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_CONTEXTMENU:
        gfx::Point cursor_pos(GetCursorScreenPoint());
        status_icon_->HandleClickEvent(cursor_pos, lparam == WM_LBUTTONDOWN);
        return TRUE;
    }
  } else if (message == WM_MENUCOMMAND) {
    int item_index = LOWORD(wparam);
    int event_flags = HIWORD(wparam);
    status_icon_->OnMenuCommand(item_index, event_flags);
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

}  // namespace brave_vpn
