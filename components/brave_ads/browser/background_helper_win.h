/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_WIN_H_

#include <windows.h>
#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "brave/components/brave_ads/browser/background_helper.h"
#include "ui/gfx/win/singleton_hwnd_observer.h"

namespace brave_ads {

class BackgroundHelperWin : public BackgroundHelper {
 public:
  BackgroundHelperWin();
  ~BackgroundHelperWin() override;

  static BackgroundHelperWin* GetInstance();

 private:
  void OnWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  std::unique_ptr<gfx::SingletonHwndObserver> singleton_hwnd_observer_;

  // BackgroundHelper impl
  bool IsForeground() const override;

  friend struct base::DefaultSingletonTraits<BackgroundHelperWin>;
  DISALLOW_COPY_AND_ASSIGN(BackgroundHelperWin);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_WIN_H_
