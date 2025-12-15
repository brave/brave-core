/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/functional/callback_helpers.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

#define SetShowCloseButton(...) SetShowCloseButton(false)
// Note: If there is no close button (and no close callback) then closing the
// dialog via the escape key will trigger the cancel callback which we **DO
// NOT** want because it closes all incognito windows when the expected behavior
// is that nothing happens.
#define SetCancelCallback(...)    \
  SetCancelCallback(__VA_ARGS__); \
  SetCloseCallback(base::DoNothing())

#include <chrome/browser/ui/views/incognito_clear_browsing_data_dialog.cc>

#undef SetCancelCallback
#undef SetShowCloseButton
