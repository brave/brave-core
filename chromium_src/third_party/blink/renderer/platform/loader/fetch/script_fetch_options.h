/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_SCRIPT_FETCH_OPTIONS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_SCRIPT_FETCH_OPTIONS_H_

#include "third_party/blink/renderer/platform/graphics/dom_node_id.h"

#define CreateFetchParameters                                          \
  CreateFetchParameters_ChromiumImpl(                                  \
      const KURL&, const SecurityOrigin*,                              \
      scoped_refptr<const DOMWrapperWorld>, CrossOriginAttributeValue, \
      const WTF::TextEncoding&, FetchParameters::DeferOption) const;   \
  void SetDOMNodeId(DOMNodeId dom_node_id);                            \
  DOMNodeId GetDOMNodeId() const { return dom_node_id_; }              \
  void SetParentScriptId(int parent_script_id);                        \
  int GetParentScriptId() const { return parent_script_id_; }          \
                                                                       \
 private:                                                              \
  DOMNodeId dom_node_id_ = kInvalidDOMNodeId;                          \
  int parent_script_id_ = 0;                                           \
                                                                       \
 public:                                                               \
  FetchParameters CreateFetchParameters

#include "src/third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"

#undef CreateFetchParameters

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_SCRIPT_FETCH_OPTIONS_H_
