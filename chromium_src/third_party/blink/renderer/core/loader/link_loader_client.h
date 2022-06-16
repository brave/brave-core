/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_LINK_LOADER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_LINK_LOADER_CLIENT_H_

namespace blink {
class HTMLLinkElement;
}

#define IsLinkCreatedByParser  \
  IsLinkCreatedByParser() = 0; \
  virtual HTMLLinkElement* GetOwner

#include "src/third_party/blink/renderer/core/loader/link_loader_client.h"

#undef IsLinkCreatedByParser

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_LINK_LOADER_CLIENT_H_
