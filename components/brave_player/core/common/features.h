/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PLAYER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_PLAYER_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace brave_player::features {

// The decision tree with theses features is here:
// https://bravesoftware.slack.com/archives/C062581P71R/p1700584069141489?thread_ts=1698262318.195859&cid=C062581P71R

COMPONENT_EXPORT(BRAVE_PLAYER_COMMON) BASE_DECLARE_FEATURE(kBravePlayer);

COMPONENT_EXPORT(BRAVE_PLAYER_COMMON) BASE_DECLARE_FEATURE(kBravePlayerRespondToAntiAdBlock);

}  // namespace brave_player::features

#endif  // BRAVE_COMPONENTS_BRAVE_PLAYER_COMMON_FEATURES_H_
