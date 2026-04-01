// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/accelerators/accelerator_manager.h"

#include <ui/base/accelerators/accelerator_manager.cc>

namespace ui {

bool AcceleratorManager::IsRegistered(const Accelerator& accelerator,
                                      AcceleratorTarget* target) const {
  const AcceleratorTargetInfo* target_info = accelerators_.Find(accelerator);
  return target_info && target_info->Contains(target);
}

}  // namespace ui
