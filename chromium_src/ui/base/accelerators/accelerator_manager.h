// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_BASE_ACCELERATORS_ACCELERATOR_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_UI_BASE_ACCELERATORS_ACCELERATOR_MANAGER_H_

// This method is added to allow callers to check if an accelerator is
// registered for the specific target. The upstream method only checks if
// the accelerator is registered for any target and it doesn't help when trying
// to check if an accelerator can be removed for a given target. Removing
// an accelerator for a target that it isn't registered for causes a DCHECK.
// This is needed in BraveBrowserView::OnAcceleratorsChanged (via FocusManager).
#define IsRegistered(...)                                     \
  IsRegistered(__VA_ARGS__, AcceleratorTarget* target) const; \
  bool IsRegistered(__VA_ARGS__)

#include <ui/base/accelerators/accelerator_manager.h>  // IWYU pragma: export
#undef IsRegistered

#endif  // BRAVE_CHROMIUM_SRC_UI_BASE_ACCELERATORS_ACCELERATOR_MANAGER_H_
