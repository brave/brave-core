// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_request_type_util.h"

namespace brave_shields {

AdBlockRequestType ToAdBlockRequestType(
    blink::mojom::ResourceType resource_type) {
  switch (resource_type) {
    case blink::mojom::ResourceType::kMainFrame:
      return AdBlockRequestType::kMainFrame;
    case blink::mojom::ResourceType::kSubFrame:
      return AdBlockRequestType::kSubFrame;
    case blink::mojom::ResourceType::kStylesheet:
      return AdBlockRequestType::kStylesheet;
    case blink::mojom::ResourceType::kScript:
      return AdBlockRequestType::kScript;
    case blink::mojom::ResourceType::kFavicon:
    case blink::mojom::ResourceType::kImage:
      return AdBlockRequestType::kImage;
    case blink::mojom::ResourceType::kFontResource:
      return AdBlockRequestType::kFont;
    case blink::mojom::ResourceType::kSubResource:
      return AdBlockRequestType::kOther;
    // An object (or embed) tag for a plugin.
    case blink::mojom::ResourceType::kObject:
      return AdBlockRequestType::kObject;
    case blink::mojom::ResourceType::kMedia:
      return AdBlockRequestType::kMedia;
    case blink::mojom::ResourceType::kXhr:
      return AdBlockRequestType::kXhr;
    // A ping request for <a ping>/sendBeacon.
    case blink::mojom::ResourceType::kPing:
      return AdBlockRequestType::kPing;
    case blink::mojom::ResourceType::kWorker:
    case blink::mojom::ResourceType::kSharedWorker:
    case blink::mojom::ResourceType::kPrefetch:
    case blink::mojom::ResourceType::kServiceWorker:
    case blink::mojom::ResourceType::kCspReport:
    case blink::mojom::ResourceType::kPluginResource:
    default:
      return AdBlockRequestType::kUnknown;
  }
}

}  // namespace brave_shields
