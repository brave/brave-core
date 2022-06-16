/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_FETCH_INITIATOR_INFO_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_FETCH_INITIATOR_INFO_H_

#include "third_party/blink/renderer/platform/graphics/dom_node_id.h"

#define referrer                             \
  referrer;                                  \
  DOMNodeId dom_node_id = kInvalidDOMNodeId; \
  int parent_script_id = 0

#include "src/third_party/blink/renderer/platform/loader/fetch/fetch_initiator_info.h"

#undef referrer

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_FETCH_INITIATOR_INFO_H_
