/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/mojom/broadcastchannel/broadcast_channel.mojom-blink.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"

#define ConnectToChannel                                                   \
  Version_;                                                                \
  {                                                                        \
    LocalDOMWindow* window = DynamicTo<LocalDOMWindow>(execution_context); \
    if (window) {                                                          \
      if (auto* origin = GetEphemeralStorageOrigin(window)) {              \
        origin_ = origin;                                                  \
      }                                                                    \
    }                                                                      \
  }                                                                        \
  provider->ConnectToChannel

#include "../../../../../../../third_party/blink/renderer/modules/broadcastchannel/broadcast_channel.cc"

#undef ConnectToChannel
