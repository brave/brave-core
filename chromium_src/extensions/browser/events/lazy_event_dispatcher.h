/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EVENTS_LAZY_EVENT_DISPATCHER_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EVENTS_LAZY_EVENT_DISPATCHER_H_

#define LazyEventDispatcher LazyEventDispatcher_Chromium
#include "../../../../../extensions/browser/events/lazy_event_dispatcher.h"
#undef LazyEventDispatcher

namespace extensions {

class BraveLazyEventDispatcher : public LazyEventDispatcher_Chromium {
  public:
    BraveLazyEventDispatcher(content::BrowserContext* browser_context,
                             DispatchFunction dispatch_function);

    void Dispatch(const Event& event,
                  const LazyContextId& dispatch_context,
                  const base::DictionaryValue* listener_filter);

  private:
    content::BrowserContext* GetTorContext(const Extension* extension);

  DISALLOW_COPY_AND_ASSIGN(BraveLazyEventDispatcher);
};

} // namespace extensions

// Use our own subclass as the real LazyEventDispatcher
#define LazyEventDispatcher BraveLazyEventDispatcher

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EVENTS_LAZY_EVENT_DISPATCHER_H_
