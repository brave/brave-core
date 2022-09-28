/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_REFERRER_SCRIPT_INFO_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_REFERRER_SCRIPT_INFO_H_

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/platform/graphics/dom_node_id.h"
#include "v8/include/v8.h"

#define BRAVE_REFERRER_SCRIPT_INFO_CONSTRUCTOR       \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {            \
    dom_node_id_ = options.GetDOMNodeId();           \
    parent_script_id_ = options.GetParentScriptId(); \
  })

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define FromV8HostDefinedOptions                                           \
  FromV8HostDefinedOptions_ChromiumImpl(v8::Local<v8::Context>,            \
                                        v8::Local<v8::Data>, const KURL&); \
  static ReferrerScriptInfo FromV8HostDefinedOptions

#define ToV8HostDefinedOptions                                          \
  ToV8HostDefinedOptions_ChromiumImpl(v8::Isolate*, const KURL&) const; \
  v8::Local<v8::Data> ToV8HostDefinedOptions

#define IsDefaultValue                                        \
  NotUsed();                                                  \
  enum HostDefinedOptionsIndex : size_t {                     \
    kBaseURL,                                                 \
    kCredentialsMode,                                         \
    kNonce,                                                   \
    kParserState,                                             \
    kReferrerPolicy,                                          \
    kDomNodeId,                                               \
    kParentScriptId,                                          \
    kLength                                                   \
  };                                                          \
  DOMNodeId GetDOMNodeId() const { return dom_node_id_; }     \
  int GetParentScriptId() const { return parent_script_id_; } \
                                                              \
 private:                                                     \
  DOMNodeId dom_node_id_ = kInvalidDOMNodeId;                 \
  int parent_script_id_ = 0;                                  \
                                                              \
 public:                                                      \
  bool IsDefaultValue
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/bindings/core/v8/referrer_script_info.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef IsDefaultValue
#undef ToV8HostDefinedOptions
#undef FromV8HostDefinedOptions
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef BRAVE_REFERRER_SCRIPT_INFO_CONSTRUCTOR

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_REFERRER_SCRIPT_INFO_H_
