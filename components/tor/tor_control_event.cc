/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_control_event.h"

#include <ostream>

#include "base/notreached.h"

namespace tor {

std::ostream& operator<<(std::ostream& os, TorControlEvent event) {
  switch (event) {
    case TorControlEvent::INVALID:
      return os << "(invalid)";
#define TOR_EVENT(N)       \
  case TorControlEvent::N: \
    return os << #N;
#include "tor_control_event_list.h"  // NOLINT
#undef TOR_EVENT
  }
  NOTREACHED();
}

}  // namespace tor
