// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_INFOBARS_WAYBACK_MACHINE_INFOBAR_CREATOR_H_
#define BRAVE_BROWSER_INFOBARS_WAYBACK_MACHINE_INFOBAR_CREATOR_H_

#include <memory>

class WaybackMachineInfoBarDelegate;

namespace infobars {
class InfoBar;
}  // namespace infobars

// Creates the platform view for a Wayback Machine infobar.
std::unique_ptr<infobars::InfoBar> CreateWaybackMachineInfoBar(
    std::unique_ptr<WaybackMachineInfoBarDelegate> delegate);

#endif  // BRAVE_BROWSER_INFOBARS_WAYBACK_MACHINE_INFOBAR_CREATOR_H_
