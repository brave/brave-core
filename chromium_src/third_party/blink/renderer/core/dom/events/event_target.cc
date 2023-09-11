/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

#define BRAVE_EVENT_TARGET_ADD_EVENT_LISTENER_INTERNAL             \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                          \
    probe::RegisterPageGraphEventListenerAdd(this, event_type,     \
                                             registered_listener); \
  })

#define BRAVE_EVENT_TARGET_REMOVE_EVENT_LISTENER_INTERNAL             \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                             \
    probe::RegisterPageGraphEventListenerRemove(this, event_type,     \
                                                registered_listener); \
  })

#define SetAttributeEventListener SetAttributeEventListener_ChromiumImpl

#include "src/third_party/blink/renderer/core/dom/events/event_target.cc"

#undef SetAttributeEventListener
#undef BRAVE_EVENT_TARGET_REMOVE_EVENT_LISTENER_INTERNAL
#undef BRAVE_EVENT_TARGET_ADD_EVENT_LISTENER_INTERNAL

namespace blink {

bool EventTarget::SetAttributeEventListener(const AtomicString& event_type,
                                            EventListener* listener) {
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  if (listener && CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph)) {
    if (RegisteredEventListener* registered_listener =
            GetAttributeRegisteredEventListener(event_type)) {
      probe::RegisterPageGraphEventListenerRemove(this, event_type,
                                                  registered_listener);
      registered_listener->SetId(RegisteredEventListener::GenerateId());
      probe::RegisterPageGraphEventListenerAdd(this, event_type,
                                               registered_listener);
    }
  }
#endif
  return EventTarget::SetAttributeEventListener_ChromiumImpl(event_type,
                                                             listener);
}

}  // namespace blink
