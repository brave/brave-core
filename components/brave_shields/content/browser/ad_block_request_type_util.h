// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_REQUEST_TYPE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_REQUEST_TYPE_UTIL_H_

#include "brave/components/brave_shields/core/browser/ad_block_request_type.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace brave_shields {

AdBlockRequestType ToAdBlockRequestType(
    blink::mojom::ResourceType resource_type);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_REQUEST_TYPE_UTIL_H_
