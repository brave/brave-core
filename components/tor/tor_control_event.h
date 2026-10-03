/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_CONTROL_EVENT_H_
#define BRAVE_COMPONENTS_TOR_TOR_CONTROL_EVENT_H_

#include <iosfwd>
#include <string_view>

#include "base/containers/fixed_flat_map.h"

namespace tor {

enum class TorControlEvent {
  INVALID,
#define TOR_EVENT(N) N,
#include "tor_control_event_list.h"  /* NOLINT(build/include_directory) */
#undef TOR_EVENT
};

inline constexpr auto kTorControlEventByName =
    base::MakeFixedFlatMap<std::string_view, TorControlEvent>({
#define TOR_EVENT(N) {#N, TorControlEvent::N},
#include "tor_control_event_list.h"  // NOLINT(build/include_directory)
#undef TOR_EVENT
    });

std::ostream& operator<<(std::ostream& os, TorControlEvent event);

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_CONTROL_EVENT_H_
