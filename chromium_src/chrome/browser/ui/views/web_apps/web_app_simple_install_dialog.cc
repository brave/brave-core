// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/models/dialog_model.h"

// Change the default button from kCancel to kNone
#define OverrideDefaultButton(__VA_ARGS__) \
  OverrideDefaultButton(ui::mojom::DialogButton::kNone)

#include <chrome/browser/ui/views/web_apps/web_app_simple_install_dialog.cc>

#undef OverrideDefaultButton
