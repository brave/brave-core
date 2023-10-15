/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Intentionally no header guards (see the comment in the overridden .h file).
// NOLINT(build/header_guard)
// no-include-guard-because-multiply-included

#include "src/net/log/net_log_event_type_list.h"  // IWYU pragma: export

// The time spent sending authentication to the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_WRITE)

// The time spent waiting for the authentication response from the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_READ)
