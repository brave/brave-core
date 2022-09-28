/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_V8_SRC_EXECUTION_ISOLATE_H_
#define BRAVE_CHROMIUM_SRC_V8_SRC_EXECUTION_ISOLATE_H_

#include "v8/include/v8-isolate.h"

#include "brave/v8/include/v8-isolate-page-graph-utils.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#define rail_mutex_                                                          \
  rail_mutex_;                                                               \
                                                                             \
 public:                                                                     \
  void set_page_graph_delegate(                                              \
      v8::page_graph::PageGraphDelegate* page_graph_delegate) {              \
    page_graph_delegate_ = page_graph_delegate;                              \
  }                                                                          \
  v8::page_graph::PageGraphDelegate* page_graph_delegate() const {           \
    return page_graph_delegate_;                                             \
  }                                                                          \
  v8::page_graph::ExecutingScript GetExecutingScript(bool include_position); \
  std::vector<v8::page_graph::ExecutingScript> GetAllExecutingScripts();     \
                                                                             \
 private:                                                                    \
  v8::page_graph::PageGraphDelegate* page_graph_delegate_ = nullptr

#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/v8/src/execution/isolate.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef rail_mutex_
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_CHROMIUM_SRC_V8_SRC_EXECUTION_ISOLATE_H_
