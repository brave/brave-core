// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/net/log/net_log_event_type_list.h"

// The time spent sending authentication to the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_WRITE)

// The time spent waiting for the authentication response from the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_READ)
