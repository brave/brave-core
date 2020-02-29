/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_control_event.h"

namespace tor {

const std::map<std::string, TorControlEvent> kTorControlEventByName = {
#define TOR_EVENT(N) {#N, TorControlEvent::N},
#include "tor_control_event_list.h"
#undef TOR_EVENT
};

const std::map<TorControlEvent, std::string> kTorControlEventByEnum = {
  {TorControlEvent::INVALID, "(invalid)"},
#define TOR_EVENT(N) {TorControlEvent::N, #N},
#include "tor_control_event_list.h"
#undef TOR_EVENT
};

}  // namespace tor
