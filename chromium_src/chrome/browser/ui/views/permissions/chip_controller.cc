// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Workaround for startup crash when GetLocationBarView() here returns nullptr.
#define BRAVE_CHIP_CONTROLLER_HIDE_CHIP if (GetLocationBarView())

#include "src/chrome/browser/ui/views/permissions/chip_controller.cc"
#undef BRAVE_CHIP_CONTROLLER_HIDE_CHIP
