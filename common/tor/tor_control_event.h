/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_CONTROL_EVENT_H_
#define BRAVE_BROWSER_TOR_TOR_CONTROL_EVENT_H_

#include <map>
#include <string>

namespace tor {

enum class TorControlEvent {
  INVALID,
#define TOR_EVENT(N) N,
#include "tor_control_event_list.h"
#undef TOR_EVENT
};

extern const std::map<std::string, TorControlEvent> kTorControlEventByName;
extern const std::map<TorControlEvent, std::string> kTorControlEventByEnum;

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_CONTROL_EVENT_H_
