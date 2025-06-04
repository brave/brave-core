/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/client_hints/client_hints.h"

#include <iostream>

#define UpdateNavigationRequestClientUaHeaders \
  UpdateNavigationRequestClientUaHeaders_ChromiumImpl

#include "src/content/browser/client_hints/client_hints.cc"

#undef UpdateNavigationRequestClientUaHeaders

namespace content {

void UpdateNavigationRequestClientUaHeaders(
    const url::Origin& origin,
    ClientHintsControllerDelegate* delegate,
    bool override_ua,
    FrameTreeNode* frame_tree_node,
    net::HttpRequestHeaders* headers,
    const std::optional<GURL>& request_url) {
  std::cout << "UpdateNavigationRequestClientUaHeaders" << std::endl;
  UpdateNavigationRequestClientUaHeaders_ChromiumImpl(
      origin, delegate, override_ua, frame_tree_node, headers, request_url);
}

}  // namespace content
