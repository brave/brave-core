// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/task_manager_view.h"

#define MenuClosed(...) MenuClosed_Unused(__VA_ARGS__)
#include <chrome/browser/ui/views/task_manager_view.cc>
#undef MenuClosed

namespace task_manager {

void TaskManagerView::MenuClosed(ui::SimpleMenuModel* source) {
  // Upstream implementation destroys menu model first, which makes menu runner
  // has reference to invalid menu model. In order to avoid this, we reset menu
  // runner first.
  menu_runner_.reset();
  menu_model_.reset();
}

}  // namespace task_manager
