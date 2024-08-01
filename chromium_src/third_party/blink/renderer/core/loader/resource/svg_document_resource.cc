/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/resource/svg_document_resource.h"

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define GetResponse() \
  GetResponse();      \
  content_->set_initiator_dom_node_id(Options().initiator_info.dom_node_id)
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/core/loader/resource/svg_document_resource.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef GetResponse
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
