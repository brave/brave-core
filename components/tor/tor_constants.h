/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_CONSTANTS_H_
#define BRAVE_COMPONENTS_TOR_TOR_CONSTANTS_H_

#include "base/files/file_path.h"

namespace tor {

// Deprecated
inline constexpr const base::FilePath::CharType kTorProfileDir[] =
    FILE_PATH_LITERAL("Tor Profile");

inline constexpr char kTorProfileID[] = "Tor::Profile";

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_CONSTANTS_H_
