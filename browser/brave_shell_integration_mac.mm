/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration_mac.h"

#include "base/apple/bundle_locations.h"
#include "chrome/browser/mac/dock.h"

namespace shell_integration::mac {

void AddIconToDock(base::OnceCallback<void(bool)> result_callback) {
  dock::AddIcon([base::apple::MainBundle() bundlePath], nullptr);

  std::move(result_callback)
      .Run(dock::ChromeIsInTheDock() == dock::ChromeInDockTrue);
}

void IsIconAddedToDock(base::OnceCallback<void(bool)> result_callback) {
  std::move(result_callback)
      .Run(dock::ChromeIsInTheDock() == dock::ChromeInDockTrue);
}

}  // namespace shell_integration::mac
