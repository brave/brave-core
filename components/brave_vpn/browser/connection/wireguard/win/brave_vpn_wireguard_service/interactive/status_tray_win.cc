/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/status_tray_win.h"

#include <commctrl.h>
#include <wrl/client.h>

#include <utility>

#include "base/files/file_path.h"
#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/brave_vpn_menu_model.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/status_icon_win.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/win/hwnd_util.h"

static const UINT kStatusIconMessage = WM_APP + 1;

namespace {
// |kBaseIconId| is 2 to avoid conflicts with plugins that hard-code id 1.
const UINT kBaseIconId = 2;

constexpr base::FilePath::CharType kStatusTrayWindowName[] =
    FILE_PATH_LITERAL("BraveVpn_StatusTrayWindow");
constexpr base::FilePath::CharType kStatusTrayWindowClass[] =
    FILE_PATH_LITERAL("BraveVpn_StatusTrayWindowClass");

const base::FilePath::CharType kBraveVpnTaskbarMessageName[] =
    FILE_PATH_LITERAL("TaskbarCreated");

gfx::Point GetCursorScreenPoint() {
  POINT pt;
  ::GetCursorPos(&pt);
  return gfx::ToFlooredPoint(
      display::win::ScreenWin::ScreenToDIPPoint(gfx::PointF(gfx::Point(pt))));
}

}  // namespace

StatusTrayWin::StatusTrayWin() : atom_(0), instance_(NULL), window_(NULL) {
  // Register our window class
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kStatusTrayWindowClass,
      &base::win::WrappedWindowProc<StatusTrayWin::WndProcStatic>, 0, 0, 0,
      NULL, NULL, NULL, NULL, NULL, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);
  CHECK(atom_);

  // If the taskbar is re-created after we start up, we have to rebuild all of
  // our icons.
  taskbar_created_message_ = RegisterWindowMessage(kBraveVpnTaskbarMessageName);

  // Create an offscreen window for handling messages for the status icons. We
  // create a hidden WS_POPUP window instead of an HWND_MESSAGE window, because
  // only top-level windows such as popups can receive broadcast messages like
  // "TaskbarCreated".
  window_.reset(CreateWindow(MAKEINTATOM(atom_), kStatusTrayWindowName,
                             WS_POPUP, 0, 0, 0, 0, 0, 0, instance_, 0));
  gfx::CheckWindowCreated(window_.get(), ::GetLastError());
  gfx::SetWindowUserData(window_.get(), this);
}

StatusTrayWin::~StatusTrayWin() {
  window_.reset();  // window must be destroyed before unregistering class.
  if (atom_) {
    UnregisterClass(MAKEINTATOM(atom_), instance_);
  }
}

bool StatusTrayWin::IconWindowExists() {
  return FindWindowEx(nullptr, nullptr, kStatusTrayWindowClass,
                      kStatusTrayWindowName) == NULL;
}

StatusIconWin* StatusTrayWin::CreateStatusIcon(const gfx::ImageSkia& image,
                                               const std::u16string& tool_tip) {
  status_icon_ = std::make_unique<StatusIconWin>(
      this, kBaseIconId, window_.get(), kStatusIconMessage);

  status_icon_->SetImage(image);
  status_icon_->SetToolTip(tool_tip);
  return status_icon_.get();
}

LRESULT CALLBACK StatusTrayWin::WndProcStatic(HWND hwnd,
                                              UINT message,
                                              WPARAM wparam,
                                              LPARAM lparam) {
  StatusTrayWin* msg_wnd =
      reinterpret_cast<StatusTrayWin*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd) {
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  } else {
    return ::DefWindowProc(hwnd, message, wparam, lparam);
  }
}

LRESULT CALLBACK StatusTrayWin::WndProc(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam) {
  if (message == taskbar_created_message_) {
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
        // Walk our icons, find which one was clicked on, and invoke its
        // HandleClickEvent() method.
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
