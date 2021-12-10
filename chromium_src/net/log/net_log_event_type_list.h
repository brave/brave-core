// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// NOTE: No header guards are used, since this file is intended to be expanded
// directly into net_log.h. DO NOT include this file anywhere else.
// The following line silences a presubmit warning that would otherwise be
// triggered by this:
// no-include-guard-because-multiply-included
// NOLINT(build/header_guard)

#include "src/net/log/net_log_event_type_list.h"

// The time spent sending authentication to the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_WRITE)

// The time spent waiting for the authentication response from the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_READ)
