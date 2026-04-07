// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"
#include "build/buildflag.h"

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
// V2: compile upstream's SidePanel implementation.
#include <chrome/browser/ui/views/side_panel/side_panel.cc>
#else
// V1: empty — upstream's SidePanel is excluded from the build.
// brave/browser/ui/views/side_panel/side_panel.cc provides the replacement.
#endif
