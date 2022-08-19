/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/dom/events/registered_event_listener.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "base/atomic_sequence_num.h"

namespace blink {

// static
int RegisteredEventListener::GenerateId() {
  static base::AtomicSequenceNumber id_sequence;
  return id_sequence.GetNext();
}

}  // namespace blink
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
