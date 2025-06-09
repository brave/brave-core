// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tabs/test/mock_tab_interface.h"

#define MockTabInterface MockTabInterfaceChromium

#include "src/chrome/browser/ui/tabs/test/mock_tab_interface.cc"  // IWYU pragma: export

#undef MockTabInterface

namespace tabs {

MockTabInterface::MockTabInterface() = default;

MockTabInterface::~MockTabInterface() = default;

}  // namespace tabs
