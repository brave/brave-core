/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/send_tab_to_self/features.cc"

#include "base/feature_override.h"

namespace send_tab_to_self {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    // This feature requires Google account at the moment, which causes crash on
    // trying to create 'Manage Devices' link.
    {kSendTabToSelfManageDevicesLink, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace send_tab_to_self
