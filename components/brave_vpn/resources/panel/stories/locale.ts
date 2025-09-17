// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import '$web-common/strings'

// On Linux Brave VPN is disabled and we don't have any strings defined.
// However, they're still needed for the Storybook (as we can't conditionally
// build stories). Thus, we add a stub for VPN strings on Linux.
declare global {
  interface Strings {
    VpnStrings: { [key: `BRAVE_VPN${string}`]: string }
  }
}

