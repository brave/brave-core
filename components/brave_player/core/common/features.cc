/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_player/core/common/features.h"

namespace brave_player::features {

BASE_FEATURE(kBravePlayer, "BravePlayer", base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBravePlayerRespondToAntiAdBlock,
             "BravePlayerRespondToAntiAdBlock",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_player::features
