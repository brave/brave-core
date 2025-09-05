/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_WIN_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_WIN_H_

#include "base/callback_list.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_ads/browser/application_state/background_helper.h"

namespace brave_ads {

class BackgroundHelperWin final : public BackgroundHelper {
 public:
  BackgroundHelperWin(const BackgroundHelperWin&) = delete;
  BackgroundHelperWin& operator=(const BackgroundHelperWin&) = delete;

  ~BackgroundHelperWin() override;

 protected:
  friend class BackgroundHelperHolder;

  BackgroundHelperWin();

 private:
  void OnWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  // BackgroundHelper:
  bool IsForeground() const override;

  base::CallbackListSubscription hwnd_subscription_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_WIN_H_
