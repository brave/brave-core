/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/background_helper_win.h"

#include <windows.h>

#include <string>
#include <vector>

#include "base/bind.h"
#include "bat/ads/configuration_info_log.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/display/screen.h"
#include "ui/views/win/hwnd_util.h"

namespace brave_ads {

namespace {

std::string GetWindowTextAsString(HWND hwnd) {
  const size_t num_chars = ::GetWindowTextLength(hwnd);
  if (!num_chars)
    return {};
  std::vector<wchar_t> tmp(num_chars + 1);
  if (!::GetWindowText(hwnd, &tmp.front(), tmp.size()))
    return {};

  return base::WideToUTF8(base::WStringPiece(&tmp.front(), num_chars));
}

void LogDisplaysInfo() {
  // using ads::DisplayInfo::Rotation;
  // using display::Display::Rotation;
  std::vector<ads::DisplayInfo> displays_info;
  display::Screen* screen = display::Screen::GetScreen();
  const auto& displays = screen->GetAllDisplays();
  for (const auto& display : displays) {
    ads::DisplayInfo info;
    switch (display.rotation()) {
      case display::Display::ROTATE_0:
        info.rotation = ads::DisplayInfo::Rotation::Rotation_0;
        break;
      case display::Display::ROTATE_90:
        info.rotation = ads::DisplayInfo::Rotation::Rotation_90;
        break;
      case display::Display::ROTATE_180:
        info.rotation = ads::DisplayInfo::Rotation::Rotation_180;
        break;
      case display::Display::ROTATE_270:
        info.rotation = ads::DisplayInfo::Rotation::Rotation_270;
        break;
    }
    displays_info.push_back(info);
  }
  ads::WriteConfigurationInfoLog(displays_info);
}

void LogBrowserWindows() {
  std::vector<ads::BrowserWindowInfo> windows_info;
  BrowserList* browser_list = BrowserList::GetInstance();
  for (const Browser* browser : *browser_list) {
    DCHECK(browser);
    ads::BrowserWindowInfo info;
    const BrowserWindow* window = browser->window();
    if (!window) {
      continue;
    }
    HWND hwnd = views::HWNDForNativeWindow(window->GetNativeWindow());
    if (!hwnd) {
      continue;
    }
    info.name = GetWindowTextAsString(hwnd);

    info.is_minimized = window->IsMinimized();
    info.is_maximized = window->IsMaximized();
    info.is_fullscreen = window->IsFullscreen();
    windows_info.push_back(info);
  }
  ads::WriteConfigurationInfoLog(windows_info);
}

void LogConfigurationInfo() {
  LogBrowserWindows();
  LogDisplaysInfo();
}

}  // namespace

BackgroundHelperWin::BackgroundHelperWin() {
  singleton_hwnd_observer_.reset(
      new gfx::SingletonHwndObserver(base::BindRepeating(
          &BackgroundHelperWin::OnWndProc, base::Unretained(this))));

  timer_.Start(FROM_HERE, base::TimeDelta::FromMinutes(1),
               base::BindRepeating(&LogConfigurationInfo));
}

BackgroundHelperWin::~BackgroundHelperWin() {}

bool BackgroundHelperWin::IsForeground() const {
  auto* browser = BrowserList::GetInstance()->GetLastActive();
  if (browser && browser->window() && browser->window()->GetNativeWindow()) {
    return ::GetForegroundWindow() ==
           views::HWNDForNativeWindow(browser->window()->GetNativeWindow());
  }

  return false;
}

void BackgroundHelperWin::OnWndProc(HWND hwnd,
                                    UINT message,
                                    WPARAM wparam,
                                    LPARAM lparam) {
  if (message != WM_ACTIVATEAPP) {
    return;
  }

  if ((BOOL)wparam) {
    TriggerOnForeground();
  } else {
    TriggerOnBackground();
  }
}

BackgroundHelperWin* BackgroundHelperWin::GetInstance() {
  return base::Singleton<BackgroundHelperWin>::get();
}

BackgroundHelper* BackgroundHelper::GetInstance() {
  return BackgroundHelperWin::GetInstance();
}

}  // namespace brave_ads
