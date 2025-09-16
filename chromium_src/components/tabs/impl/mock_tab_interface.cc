// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/tabs/public/mock_tab_interface.h"

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#define MockTabInterface MockTabInterfaceChromium
#endif  // !BUILDFLAG(IS_ANDROID)

#include <components/tabs/impl/mock_tab_interface.cc>

#if !BUILDFLAG(IS_ANDROID)
#undef MockTabInterface

namespace tabs {

MockTabInterface::MockTabInterface() = default;
MockTabInterface::~MockTabInterface() = default;

}  // namespace tabs

#endif  // !BUILDFLAG(IS_ANDROID)
