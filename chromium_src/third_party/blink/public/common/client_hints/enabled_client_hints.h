/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_ENABLED_CLIENT_HINTS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_ENABLED_CLIENT_HINTS_H_

#include "services/network/public/mojom/web_client_hints_types.mojom-shared.h"

// We can't do a `#define SetIsEnabled SetIsEnabled_Unused` because there are
// two different signatures for that method, so we redefine IsEnabled() instead
// and make the most of that to squeeze those methods there instead.
#define IsEnabled                                                         \
  IsEnabled(network::mojom::WebClientHintsType type) const;               \
  void SetIsEnabled_ChromiumImpl(network::mojom::WebClientHintsType type, \
                                 bool should_send);                       \
  bool IsEnabled_Unused

#include "src/third_party/blink/public/common/client_hints/enabled_client_hints.h"  // IWYU pragma: export

#undef IsEnabled

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_ENABLED_CLIENT_HINTS_H_
