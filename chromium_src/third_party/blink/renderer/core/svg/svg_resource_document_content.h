/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_RESOURCE_DOCUMENT_CONTENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_RESOURCE_DOCUMENT_CONTENT_H_

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/platform/graphics/dom_node_id.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define AsyncLoadingFinished(...)                                            \
  AsyncLoadingFinished(__VA_ARGS__);                                         \
                                                                             \
 public:                                                                     \
  blink::DOMNodeId initiator_dom_node_id() const {                           \
    return initiator_dom_node_id_;                                           \
  }                                                                          \
  blink::DOMNodeId set_initiator_dom_node_id(blink::DOMNodeId dom_node_id) { \
    return initiator_dom_node_id_ = dom_node_id;                             \
  }                                                                          \
                                                                             \
 private:                                                                    \
  blink::DOMNodeId initiator_dom_node_id_ = blink::kInvalidDOMNodeId
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/core/svg/svg_resource_document_content.h"  // IWYU pragma: export

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef AsyncLoadingFinished
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_RESOURCE_DOCUMENT_CONTENT_H_
