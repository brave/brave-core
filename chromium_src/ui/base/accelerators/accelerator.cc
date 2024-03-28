// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/accelerators/accelerator.h"

// Upstream doesn't support accelerators with Control+Alt - we do, but only for
// user defined shortcuts.
#define BRAVE_UI_BASE_ACCELERATOR_APPLY_LONG_FORM_MODIFIERS_ \
  if (IsCtrlDown() && IsAltDown())                           \
    result = ApplyModifierToAcceleratorString(result, IDS_APP_ALT_KEY);

#include "src/ui/base/accelerators/accelerator.cc"

#undef BRAVE_UI_BASE_ACCELERATOR_APPLY_LONG_FORM_MODIFIERS_
