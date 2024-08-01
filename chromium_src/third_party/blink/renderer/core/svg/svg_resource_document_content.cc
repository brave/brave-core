/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/svg/svg_resource_document_content.h"

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image_chrome_client.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define InvalidateContainer(...)                  \
  InvalidateContainer_Unused();                   \
  DOMNodeId InitiatorDomNodeId() const override { \
    return content_->initiator_dom_node_id();     \
  }                                               \
  void InvalidateContainer(__VA_ARGS__)
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/core/svg/svg_resource_document_content.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef InvalidateContainer
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
