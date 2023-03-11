/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_EVENT_TARGET_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_EVENT_TARGET_H_

#define dispatchEventForBindings                                              \
  NotUsed();                                                                  \
  bool SetAttributeEventListener_ChromiumImpl(const AtomicString& event_type, \
                                              EventListener*);                \
  bool dispatchEventForBindings

#include "src/third_party/blink/renderer/core/dom/events/event_target.h"  // IWYU pragma: export

#undef dispatchEventForBindings

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_EVENT_TARGET_H_
