/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host_win.h"

#include <dwmapi.h>

#include "base/feature_list.h"
#include "base/win/windows_types.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "skia/ext/skia_utils_win.h"

#define BrowserDesktopWindowTreeHostWin \
  BrowserDesktopWindowTreeHostWin_ChromiumImpl
#define CreateBrowserDesktopWindowTreeHost \
  CreateBrowserDesktopWindowTreeHost_Unused

#include "src/chrome/browser/ui/views/frame/browser_desktop_window_tree_host_win.cc"

#undef BrowserDesktopWindowTreeHostWin
#undef CreateBrowserDesktopWindowTreeHost

bool BrowserDesktopWindowTreeHostWin::PreHandleMSG(UINT message,
                                                   WPARAM w_param,
                                                   LPARAM l_param,
                                                   LRESULT* result) {
  switch (message) {
    case WM_NCCREATE: {
      // Cloak the window on creation to prevent a white flash.
      if (!is_cloacked_ && base::FeatureList::IsEnabled(
                               features::kBraveWorkaroundNewWindowFlash)) {
        const BOOL cloak = TRUE;
        is_cloacked_ = SUCCEEDED(DwmSetWindowAttribute(GetHWND(), DWMWA_CLOAK,
                                                       &cloak, sizeof(cloak)));
      }
      break;
    }
    case WM_NCPAINT: {
      // If the window is cloacked, fill it with the toolbar color and uncloak.
      if (is_cloacked_ && base::FeatureList::IsEnabled(
                              features::kBraveWorkaroundNewWindowFlash)) {
        HWND hwnd = GetHWND();
        HDC dc = GetWindowDC(hwnd);
        RECT window_rect;
        GetWindowRect(hwnd, &window_rect);
        const RECT fill_rect = {
            0,
            0,
            window_rect.right - window_rect.left,
            window_rect.bottom - window_rect.top,
        };

        SkColor bg_color = GetToolbarColor();
        HBRUSH brush = ::CreateSolidBrush(skia::SkColorToCOLORREF(bg_color));
        ::FillRect(dc, &fill_rect, brush);
        ::DeleteObject(brush);
        ::ReleaseDC(hwnd, dc);
        const BOOL cloak = FALSE;
        is_cloacked_ = !SUCCEEDED(
            DwmSetWindowAttribute(hwnd, DWMWA_CLOAK, &cloak, sizeof(cloak)));
      }
      break;
    }
  }
  return BrowserDesktopWindowTreeHostWin_ChromiumImpl::PreHandleMSG(
      message, w_param, l_param, result);
}

SkColor BrowserDesktopWindowTreeHostWin::GetBackgroundColor(
    SkColor requested_color) const {
  if (requested_color == SK_ColorTRANSPARENT ||
      !base::FeatureList::IsEnabled(features::kBraveWorkaroundNewWindowFlash)) {
    return requested_color;
  }

  return GetToolbarColor();
}

SkColor BrowserDesktopWindowTreeHostWin::GetToolbarColor() const {
  CHECK(base::FeatureList::IsEnabled(features::kBraveWorkaroundNewWindowFlash));
  return GetWidget()->GetColorProvider()->GetColor(kColorToolbar);
}

// static
BrowserDesktopWindowTreeHost*
BrowserDesktopWindowTreeHost::CreateBrowserDesktopWindowTreeHost(
    views::internal::NativeWidgetDelegate* native_widget_delegate,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura,
    BrowserView* browser_view,
    BrowserFrame* browser_frame) {
  return new BrowserDesktopWindowTreeHostWin(native_widget_delegate,
                                             desktop_native_widget_aura,
                                             browser_view, browser_frame);
}
