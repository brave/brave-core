// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/page_action/test_support/mock_page_action_controller.h"

#define MockPageActionController MockPageActionController_Chromium
#include <chrome/browser/ui/views/page_action/test_support/mock_page_action_controller.cc>
#undef MockPageActionController

namespace page_actions {

MockPageActionController::MockPageActionController() = default;
MockPageActionController::~MockPageActionController() = default;

}  // namespace page_actions
