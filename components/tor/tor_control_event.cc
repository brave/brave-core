/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_control_event.h"

namespace tor {

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const std::map<std::string, TorControlEvent>
    kTorControlEventByName = {
#define TOR_EVENT(N) {#N, TorControlEvent::N},
#include "tor_control_event_list.h"  // NOLINT
#undef TOR_EVENT
};

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const std::map<TorControlEvent, std::string>
    kTorControlEventByEnum = {
        {TorControlEvent::INVALID, "(invalid)"},
#define TOR_EVENT(N) {TorControlEvent::N, #N},
#include "tor_control_event_list.h"  // NOLINT
#undef TOR_EVENT
};

}  // namespace tor
