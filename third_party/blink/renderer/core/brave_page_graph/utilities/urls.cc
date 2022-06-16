/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/urls.h"

#include "third_party/blink/renderer/platform/weborigin/kurl.h"

using ::blink::KURL;
namespace brave_page_graph {

KURL NormalizeUrl(const KURL& url) {
  KURL url_copy(url);
  if (!url_copy.ProtocolIsInHTTPFamily()) {
    return url_copy;
  }

  // Sloppy normlization to match across HSTS enduced HTTPS upgrades.
  url_copy.SetProtocol("https");

  // Chromium doesn't care about fragment identifiers when caching, so
  // neither should we.
  url_copy.RemoveFragmentIdentifier();
  return url_copy;
}

}  // namespace brave_page_graph
