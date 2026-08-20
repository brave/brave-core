// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_COMMON_SWITCHES_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_COMMON_SWITCHES_H_

namespace containers::switches {

// Specifies the container to use for the tabs passed on the command line. On
// its own it resolves an existing container by name; with `kTemporaryContainer`
// it names the temporary container instead.
inline constexpr char kContainer[] = "container";

// Opens the tabs passed on the command line in a temporary container.
inline constexpr char kTemporaryContainer[] = "temporary-container";

}  // namespace containers::switches

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_COMMON_SWITCHES_H_
