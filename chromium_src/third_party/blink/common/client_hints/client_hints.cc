/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/features.h"

#define BRAVE_CLIENT_HINTS_IS_CLIENT_HINT_SENT_BY_DEFAULT \
  switch (type) {                                         \
    case network::mojom::WebClientHintsType::kUA:         \
    case network::mojom::WebClientHintsType::kUAMobile:   \
    case network::mojom::WebClientHintsType::kUAPlatform: \
      break;                                              \
    default:                                              \
      return false;                                       \
  }

#include "src/third_party/blink/common/client_hints/client_hints.cc"
#undef BRAVE_CLIENT_HINTS_IS_CLIENT_HINT_SENT_BY_DEFAULT
