// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import API, {ConversationEntriesUIState, defaultConversationEntriesUIState} from './api_conversation_entries_ui'
import * as Mojom from '../common/mojom'
import useAPIState from '../common/useAPIState'

export type ConversationEntriesContext = ConversationEntriesUIState & {
  // Access to interface functions, optional because it will not be available
  // in storybook.
  conversationHandler?: Mojom.UntrustedConversationHandlerRemote
  uiHandler?: Mojom.UntrustedUIHandlerRemote
}

const defaultContext: ConversationEntriesContext = {
  ...defaultConversationEntriesUIState
}

export const ConversationEntriesReactContext =
  React.createContext<ConversationEntriesContext>(defaultContext)

export function ConversationEntriesContextProvider(props: React.PropsWithChildren) {
  const api = API.getInstance()
  const context = useAPIState(api, defaultContext)

  const store: ConversationEntriesContext = {
    ...context,
    conversationHandler: api.conversationHandler,
    uiHandler: api.uiHandler
  }

  return (
    <ConversationEntriesReactContext.Provider value={store}>
      {props.children}
    </ConversationEntriesReactContext.Provider>
  )
}

export function useConversationEntriesContext() {
  return React.useContext(ConversationEntriesReactContext)
}
