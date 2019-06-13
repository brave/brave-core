/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/browser/events/lazy_event_dispatcher.h"

// Our .h file creates a masquerade for LazyEventDispatcher. Switch back to the
// Chromium one for the Chromium implementation.
#undef LazyEventDispatcher
#define LazyEventDispatcher LazyEventDispatcher_Chromium

#include "../../../../../extensions/browser/events/lazy_event_dispatcher.cc"  // NOLINT

// Make it clear which class we mean here.
#undef LazyEventDispatcher

namespace extensions {

BraveLazyEventDispatcher::BraveLazyEventDispatcher(
    content::BrowserContext* browser_context,
    DispatchFunction dispatch_function)
  : LazyEventDispatcher_Chromium(browser_context, dispatch_function) {
}

BrowserContext* BraveLazyEventDispatcher::GetTorContext(
    const Extension* extension) {
  if (!IncognitoInfo::IsSplitMode(extension))
    return nullptr;

  ExtensionsBrowserClient* browser_client = ExtensionsBrowserClient::Get();

  if (!browser_client->HasTorContext(browser_context_))
    return nullptr;
  return browser_client->GetTorContext(browser_context_);
}

void BraveLazyEventDispatcher::Dispatch(const Event& event,
    const LazyContextId& dispatch_context,
    const base::DictionaryValue* listener_filter) {
  // Dispatch for original and OTR contexts
  LazyEventDispatcher_Chromium::Dispatch(event,
      dispatch_context,
      listener_filter);

  const Extension* extension = ExtensionRegistry::Get(browser_context_)
                                   ->enabled_extensions()
                                   .GetByID(dispatch_context.extension_id());
  if (!extension)
    return;

  // Dispatch for tor context
  BrowserContext* tor_context = GetTorContext(extension);
  if (!tor_context)
    return;

  LazyContextId tor_context_id(dispatch_context);
  tor_context_id.set_browser_context(tor_context);
  if (QueueEventDispatch(event, tor_context_id, extension,
        listener_filter)) {
    RecordAlreadyDispatched(tor_context_id);
  }
}

}  // namespace extensions
