// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FEATURES_H_

#include "base/feature_list.h"

namespace brave_transfer::features {

// Brave Transfer: brave://transfer, the toolbar button and local-network
// transfers between the user's devices. Off by default until the M2 security
// review. Enable with --enable-features=BraveTransfer.
BASE_DECLARE_FEATURE(kBraveTransfer);

bool IsBraveTransferEnabled();

}  // namespace brave_transfer::features

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FEATURES_H_
