/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_H_

namespace net {

// HTTP status code 418 "I'm a teapot" is an April Fools' joke from the IETF's
// RFC 2324, Hyper Text Coffee Pot Control Protocol (HTCPCP/1.0).
constexpr int kHttpImATeapot = 418;

}  // namespace net

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_H_
