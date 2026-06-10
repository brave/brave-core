/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/chrome/browser/ui/page_actions/page_action_triggers.h"

#include "ui/events/event_constants.h"

#include <chrome/browser/ui/page_actions/page_action_triggers.cc>

namespace page_actions {

DEFINE_UI_CLASS_PROPERTY_KEY(int,
                             kBravePageActionEventFlagKey,
                             ui::EF_NONE)
}  // namespace page_actions