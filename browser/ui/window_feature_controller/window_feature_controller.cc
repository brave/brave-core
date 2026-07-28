/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/layout_constants.h"

// TODO(https://github.com/brave/brave-browser/issues/57601): The vertical
// tabs check should be moved here instead of being done separately in
// WindowFeatureController to avoid adding external dependency to upstream
// target.
bool BraveDisablesImmersiveFullscreenMode() {
  return tabs::UseCompactHorizontalTabs();
}
