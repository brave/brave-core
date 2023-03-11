/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_REGISTERED_EVENT_LISTENER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_REGISTERED_EVENT_LISTENER_H_

#include "brave/components/brave_page_graph/common/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define SetBlockedEventWarningEmitted \
  NotUsed();                          \
  static int GenerateId();            \
  void SetId(int id) { id_ = id; }    \
  int Id() const { return id_; }      \
                                      \
 private:                             \
  int id_;                            \
                                      \
 public:                              \
  void SetBlockedEventWarningEmitted
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/core/dom/events/registered_event_listener.h"  // IWYU pragma: export

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef SetBlockedEventWarningEmitted
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_REGISTERED_EVENT_LISTENER_H_
