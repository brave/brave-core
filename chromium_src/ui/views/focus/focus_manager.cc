// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/views/focus/focus_manager.h"

#include <ui/views/focus/focus_manager.cc>

namespace views {

bool FocusManager::IsAcceleratorRegistered(
    const ui::Accelerator& accelerator,
    ui::AcceleratorTarget* target) const {
  return accelerator_manager_.IsRegistered(accelerator, target);
}

}  // namespace views
