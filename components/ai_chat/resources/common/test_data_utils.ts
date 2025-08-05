// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

const eventTemplate: Mojom.ConversationEntryEvent = {
  // Mojom doesn't want us to have these undefined properties (it wants the properties
  // not to be present) but typescript, based on the generated mojom JS, needs them.
  completionEvent: undefined,
  searchQueriesEvent: undefined,
  searchStatusEvent: undefined,
  selectedLanguageEvent: undefined,
  conversationTitleEvent: undefined,
  sourcesEvent: undefined,
  contentReceiptEvent: undefined,
  toolUseEvent: undefined
}

export function getEventTemplate() {
  return { ...eventTemplate }
}

export function getCompletionEvent(text: string): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    completionEvent: { completion: text }
  }
}

export function getWebSourcesEvent(sources: Mojom.WebSource[]): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    sourcesEvent: { sources }
  }
}

