// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_CHROMIUM_SRC_NET_LOG_NET_LOG_EVENT_TYPE_LIST_H_
#define BRAVE_CHROMIUM_SRC_NET_LOG_NET_LOG_EVENT_TYPE_LIST_H_

#include "src/net/log/net_log_event_type_list.h"

// The time spent sending authentication to the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_WRITE)

// The time spent waiting for the authentication response from the SOCKS server
EVENT_TYPE(SOCKS5_AUTH_READ)

#endif  // BRAVE_CHROMIUM_SRC_NET_LOG_NET_LOG_EVENT_TYPE_LIST_H_
