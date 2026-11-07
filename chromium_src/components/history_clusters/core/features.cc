/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/history_clusters/core/features.cc>

namespace history_clusters {

namespace {

// enabled_by_default_desktop_only would otherwise trip
// -Wunused-const-variable now that the plaster above forces kJourneys off
// regardless of platform; referencing it here keeps the upstream declaration
// untouched.
[[maybe_unused]] constexpr auto& kUnusedEnabledByDefaultDesktopOnly =
    enabled_by_default_desktop_only;

}  // namespace

}  // namespace history_clusters
