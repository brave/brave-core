/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/background_helper/background_helper_linux.h"

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/xproto_util.h"

namespace brave_ads {

BackgroundHelperLinux::BackgroundHelperLinux() {
  BrowserList::AddObserver(this);
  OnBrowserSetLastActive(BrowserList::GetInstance()->GetLastActive());
}

BackgroundHelperLinux::~BackgroundHelperLinux() {
  BrowserList::RemoveObserver(this);
}

bool BackgroundHelperLinux::IsForeground() const {
  x11::Window x11_window = x11::Window::None;
  x11::GetProperty(ui::GetX11RootWindow(), x11::GetAtom("_NET_ACTIVE_WINDOW"),
                   &x11_window);

  for (auto* browser : *BrowserList::GetInstance()) {
    auto window =
        browser->window()->GetNativeWindow()->GetHost()->GetAcceleratedWidget();
    if (x11_window == static_cast<x11::Window>(window)) {
      return true;
    }
  }

  return false;
}

void BackgroundHelperLinux::OnBrowserSetLastActive(Browser* browser) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&BackgroundHelperLinux::TriggerOnForeground, AsWeakPtr()));
}

void BackgroundHelperLinux::OnBrowserNoLongerActive(Browser* browser) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&BackgroundHelperLinux::TriggerOnBackground, AsWeakPtr()));
}

}  // namespace brave_ads
