/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_PAGE_GRAPH_H_
#define BRAVE_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_PAGE_GRAPH_H_

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/public/platform/web_url.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH),
              "Page Graph buildflag should be enabled");

namespace blink {

class WebPageGraph {
 public:
  virtual ~WebPageGraph() = default;

  virtual void RegisterResourceBlockAd(const WebURL& url,
                                       const std::string& rule) = 0;
  virtual void RegisterResourceBlockTracker(const WebURL& url,
                                            const std::string& host) = 0;
  virtual void RegisterResourceBlockJavaScript(const WebURL& url) = 0;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_PAGE_GRAPH_H_
