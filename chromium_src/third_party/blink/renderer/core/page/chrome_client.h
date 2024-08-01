/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CHROME_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CHROME_CLIENT_H_

#include "third_party/blink/renderer/platform/graphics/dom_node_id.h"

#define GetScreenInfos                           \
  BraveGetScreenInfos(LocalFrame& frame) const;  \
  virtual DOMNodeId InitiatorDomNodeId() const { \
    return kInvalidDOMNodeId;                    \
  }                                              \
  virtual const display::ScreenInfos& GetScreenInfos

#include "src/third_party/blink/renderer/core/page/chrome_client.h"  // IWYU pragma: export

#undef GetScreenInfos

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CHROME_CLIENT_H_
