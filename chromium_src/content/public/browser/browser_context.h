/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_

namespace content {
class RenderViewHost;
class SiteInstance;
}  // namespace content

// Brave-specific: allows embedder to request deletion of an in-memory
// StoragePartition.
#define BRAVE_BROWSER_CONTEXT_H \
  void ClearEphemeralStorageForHost(RenderViewHost*, SiteInstance*);

#include "../../../../../content/public/browser/browser_context.h"

#undef BRAVE_BROWSER_CONTEXT_H

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
