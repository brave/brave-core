/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_BRAVE_ISOLATED_WORLDS_H_
#define BRAVE_COMMON_BRAVE_ISOLATED_WORLDS_H_

#include "content/public/common/isolated_world_ids.h"

enum BraveIsolatedWorldIDs {
    // Isolated world ID for Greaselion (Google Translate reserves END + 1)
    ISOLATED_WORLD_ID_GREASELION = content::ISOLATED_WORLD_ID_CONTENT_END + 2,
};

#endif  // BRAVE_COMMON_BRAVE_ISOLATED_WORLDS_H_
