/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

#define BRAVE_EVENT_TARGET_ADD_EVENT_LISTENER_INTERNAL              \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                           \
    probe::RegisterPageGraphEventListenerAdd(this, event_type,      \
                                             &registered_listener); \
  })

#define BRAVE_EVENT_TARGET_REMOVE_EVENT_LISTENER_INTERNAL              \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                              \
    probe::RegisterPageGraphEventListenerRemove(this, event_type,      \
                                                &registered_listener); \
  })

#define BRAVE_EVENT_TARGET_SET_ATTRIBUTE_EVENT_LISTENER                  \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                                \
    if (CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph)) {     \
      probe::RegisterPageGraphEventListenerRemove(this, event_type,      \
                                                  registered_listener);  \
      registered_listener->SetId(RegisteredEventListener::GenerateId()); \
      probe::RegisterPageGraphEventListenerAdd(this, event_type,         \
                                               registered_listener);     \
    }                                                                    \
  })

#include "src/third_party/blink/renderer/core/dom/events/event_target.cc"
