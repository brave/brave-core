/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/background_helper/background_helper_linux.h"

// Something in (or included in) chrome/browser/ui/browser.h causes a build
// error when ui/base/x/x11_util.h is included after it:
// ../../ui/base/x/x11_util.h:367:68: error: invalid operands to binary
// expression ('x11::EventMask' and 'x11::EventMask')
//  367 |     x11::EventMask event_mask = x11::EventMask::SubstructureNotify |
//      |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^
//  368 |                                 x11::EventMask::SubstructureRedirect);
//      |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// clang-format off
#include "ui/base/x/x11_util.h"
// clang-format on

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/gfx/x/atom_cache.h"
#include "ui/gfx/x/connection.h"

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
  x11::Connection::Get()->GetPropertyAs(
      ui::GetX11RootWindow(), x11::GetAtom("_NET_ACTIVE_WINDOW"), &x11_window);

  for (Browser* browser : *BrowserList::GetInstance()) {
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
      FROM_HERE, base::BindOnce(&BackgroundHelperLinux::TriggerOnForeground,
                                weak_ptr_factory_.GetWeakPtr()));
}

void BackgroundHelperLinux::OnBrowserNoLongerActive(Browser* browser) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BackgroundHelperLinux::TriggerOnBackground,
                                weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace brave_ads
