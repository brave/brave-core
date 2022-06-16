/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"

#define CreateFetchParameters CreateFetchParameters_ChromiumImpl

#include "src/third_party/blink/renderer/platform/loader/fetch/script_fetch_options.cc"

#undef CreateFetchParameters

namespace blink {

FetchParameters ScriptFetchOptions::CreateFetchParameters(
    const KURL& url,
    const SecurityOrigin* security_origin,
    scoped_refptr<const DOMWrapperWorld> world_for_csp,
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

void ScriptFetchOptions::SetParentScriptId(int parent_script_id) {
  parent_script_id_ = parent_script_id;
}

}  // namespace blink
