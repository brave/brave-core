/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_isolated_world_info.h"

namespace blink {

void AdjustedSetIsolatedWorldInfo(int32_t world_id,
                                  const blink::WebIsolatedWorldInfo& info) {
  blink::WebIsolatedWorldInfo new_info(info);
  // Limit all network requst to the security origin.
  new_info.content_security_policy =
      "default-src 'self' 'unsafe-eval' 'unsafe-inline'";
  blink::SetIsolatedWorldInfo(world_id, new_info);
}
}  // namespace blink

#define SetIsolatedWorldInfo AdjustedSetIsolatedWorldInfo
#include "../../../../../../components/translate/content/renderer/isolated_world_util.cc"
