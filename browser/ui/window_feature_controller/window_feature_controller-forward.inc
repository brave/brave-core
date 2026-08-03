/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/weak_ptr.h"

class VerticalTabController;

bool BraveDisablesImmersiveFullscreenMode(
    const base::WeakPtr<VerticalTabController>& vertical_tab_controller);
bool BraveShouldShowTitlebar(
    const base::WeakPtr<VerticalTabController>& vertical_tab_controller);
