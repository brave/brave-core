/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_CONTROL_EVENT_H_
#define BRAVE_COMPONENTS_TOR_TOR_CONTROL_EVENT_H_

#include <map>
#include <string>

namespace tor {

enum class TorControlEvent {
  INVALID,
#define TOR_EVENT(N) N,
#include "tor_control_event_list.h"  /* NOLINT(build/include_directory) */
#undef TOR_EVENT
};

extern const std::map<std::string, TorControlEvent> kTorControlEventByName;
extern const std::map<TorControlEvent, std::string> kTorControlEventByEnum;

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_CONTROL_EVENT_H_
