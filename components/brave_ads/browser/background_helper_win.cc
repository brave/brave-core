/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/background_helper_win.h"

#include "base/bind.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/views/win/hwnd_util.h"

namespace brave_ads {

BackgroundHelperWin::BackgroundHelperWin() {
  singleton_hwnd_observer_.reset(
      new gfx::SingletonHwndObserver(base::BindRepeating(
          &BackgroundHelperWin::OnWndProc, base::Unretained(this))));
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
