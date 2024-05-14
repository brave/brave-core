/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/dom/document.h"

#include "brave/components/brave_page_graph/common/buildflags.h"

#define ProcessJavaScriptUrl ProcessJavaScriptUrl_ChromiumImpl
#include "src/third_party/blink/renderer/core/dom/document.cc"
#undef ProcessJavaScriptUrl

namespace blink {

void Document::ProcessJavaScriptUrl(const KURL& url,
                                    const DOMWrapperWorld* world) {
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  probe::RegisterPageGraphJavaScriptUrl(this, url);
#endif
  ProcessJavaScriptUrl_ChromiumImpl(url, world);
}

}  // namespace blink
