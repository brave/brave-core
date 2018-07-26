// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_NET_BASE_URL_AUTH_UTIL_H_
#define BRAVE_NET_BASE_URL_AUTH_UTIL_H_

#include "../../../../net/base/url_util.h"

#include "base/strings/string_piece_forward.h"
#include "net/base/net_export.h"

namespace net {

NET_EXPORT bool ParseAuthHostAndPort(
    base::StringPiece input,
    std::string* username,
    std::string* password,
    std::string* host,
    int* port);

}

#endif  // BRAVE_NET_BASE_URL_AUTH_UTIL_H_
