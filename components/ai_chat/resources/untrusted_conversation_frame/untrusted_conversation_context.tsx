// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import API, {ConversationEntriesUIState, defaultConversationEntriesUIState} from './untrusted_conversation_frame_api'
import * as Mojom from '../common/mojom'
import useAPIState from '../common/useAPIState'

/**
 * UI state and functions exposed to UI in the untrusted conversation entries frame
 */
export type UntrustedConversationContext = ConversationEntriesUIState & {
  // Access to interface functions, optional because it will not be available
  // in storybook.
  conversationHandler?: Mojom.UntrustedConversationHandlerRemote
  uiHandler?: Mojom.UntrustedUIHandlerRemote
  parentUiFrame?: Mojom.ParentUIFrameRemote
}

const defaultContext: UntrustedConversationContext = {
  ...defaultConversationEntriesUIState
}

export const UntrustedConversationReactContext =
  React.createContext<UntrustedConversationContext>(defaultContext)

export function UntrustedConversationContextProvider(props: React.PropsWithChildren) {
  const api = API.getInstance()
  const context = useAPIState(api, defaultContext)

  const store: UntrustedConversationContext = {
    ...context,
    conversationHandler: api.conversationHandler,
    uiHandler: api.uiHandler,
    parentUiFrame: api.parentUIFrame
  }

  return (
    <UntrustedConversationReactContext.Provider value={store}>
      {props.children}
    </UntrustedConversationReactContext.Provider>
  )
}

export function useUntrustedConversationContext() {
  return React.useContext(UntrustedConversationReactContext)
}
