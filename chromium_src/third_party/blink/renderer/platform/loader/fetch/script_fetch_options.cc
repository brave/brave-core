/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define CreateFetchParameters CreateFetchParameters_ChromiumImpl
#endif

#include "src/third_party/blink/renderer/platform/loader/fetch/script_fetch_options.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef CreateFetchParameters
#endif

namespace blink {

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
FetchParameters ScriptFetchOptions::CreateFetchParameters(
    const KURL& url,
    const SecurityOrigin* security_origin,
    const DOMWrapperWorld* world_for_csp,
    CrossOriginAttributeValue cross_origin,
    const WTF::TextEncoding& encoding,
    FetchParameters::DeferOption defer) const {
  auto params = CreateFetchParameters_ChromiumImpl(
      url, security_origin, world_for_csp, cross_origin, encoding, defer);
  params.MutableOptions().initiator_info.dom_node_id = dom_node_id_;
  params.MutableOptions().initiator_info.parent_script_id = parent_script_id_;
  return params;
}

void ScriptFetchOptions::SetDOMNodeId(DOMNodeId dom_node_id) {
  dom_node_id_ = dom_node_id;
}

DOMNodeId ScriptFetchOptions::GetDOMNodeId() const {
  return dom_node_id_;
}

void ScriptFetchOptions::SetParentScriptId(int parent_script_id) {
  parent_script_id_ = parent_script_id;
}

int ScriptFetchOptions::GetParentScriptId() const {
  return parent_script_id_;
}
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

}  // namespace blink
