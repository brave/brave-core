// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/page_action/test_support/mock_page_action_model.h"

#define MockPageActionModel MockPageActionModel_Chromium
#include <chrome/browser/ui/views/page_action/test_support/mock_page_action_model.cc>
#undef MockPageActionModel

namespace page_actions {

MockPageActionModel::MockPageActionModel() = default;
MockPageActionModel::~MockPageActionModel() = default;

}  // namespace page_actions
