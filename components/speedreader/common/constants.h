// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_

#include "content/public/common/isolated_world_ids.h"

namespace speedreader {

constexpr const int kIsolatedWorldId =
    content::ISOLATED_WORLD_ID_CONTENT_END + 9;

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_
