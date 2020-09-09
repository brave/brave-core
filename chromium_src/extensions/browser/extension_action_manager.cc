// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "extensions/common/constants.h"

// Avoid a jump from small image to large image when the non-default
// image is set.
#define BRAVE_GET_EXTENSION_ACTION                     \
      (extension.id() == brave_rewards_extension_id || \
       extension.id() == brave_extension_id) ?         \
        kBraveActionGraphicSize :

#include "../../../../extensions/browser/extension_action_manager.cc"
#undef BRAVE_GET_EXTENSION_ACTION
